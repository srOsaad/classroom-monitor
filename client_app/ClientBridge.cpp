#include "ClientBridge.h"
#include "protocol.h"
#include <QClipboard>
#include <QDebug>
#include <QGuiApplication>
#include <QProcess>
#include <QScreen>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

ClientBridge::ClientBridge(QObject *parent)
    : QObject(parent)
    , m_status(QStringLiteral("Idle"))
    , m_client(new WebSocketClient(this))
    , m_heartbeat(new HeartbeatService(m_client, 5000, this))
    , m_discovery(new DiscoveryListener(this))
    , m_capturer(new ScreenCapturer(m_client, this))
    , m_audioManager(new ClientAudioManager(m_client, this))
{
    connect(m_discovery, &DiscoveryListener::adminFound,
            this, &ClientBridge::onAdminFound);

    connect(m_client, &WebSocketClient::authenticated, this, [this]() {
        m_discovery->stop();
        m_heartbeat->start();
        m_remoteInputCleanupDone = false;
        setStatus(QStringLiteral("Connected"));

        if (QScreen *screen = QGuiApplication::primaryScreen()) {
            const QRect g = screen->geometry();
            m_client->sendText(makeScreenSizeMessage(g.width(), g.height()));
            qInfo() << "[Client] Sent screen size" << g.width() << "x" << g.height();
        }
    });

    connect(m_client, &WebSocketClient::authFailed, this, [this](const QString &r) {
        setStatus(QStringLiteral("Auth failed: ") + r);
    });

    connect(m_client, &WebSocketClient::disconnectedFromAdmin, this, [this]() {
        m_heartbeat->stop();
        m_capturer->stop();
        m_audioManager->stopMicCapture();
        m_audioManager->stopPlayback();

        if (!m_remoteInputCleanupDone) {
#ifdef Q_OS_WIN
            if (m_mouseLeftDown)   { mouse_event(MOUSEEVENTF_LEFTUP,   0, 0, 0, 0); m_mouseLeftDown = false; }
            if (m_mouseRightDown)  { mouse_event(MOUSEEVENTF_RIGHTUP,  0, 0, 0, 0); m_mouseRightDown = false; }
            if (m_mouseMiddleDown) { mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, 0); m_mouseMiddleDown = false; }
            for (int key : std::as_const(m_pressedKeys))
                keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
#endif
            m_pressedKeys.clear();
            m_remoteInputCleanupDone = true;
        }

        setStatus(QStringLiteral("Searching for admin..."));
        startDiscovery();
    });

    connect(m_client, &WebSocketClient::textMessageReceived,
            this, &ClientBridge::onTextMessage);
    connect(m_client, &WebSocketClient::binaryMessageReceived,
            this, &ClientBridge::onBinaryMessage);
}

void ClientBridge::setStatus(const QString &s) {
    if (m_status == s) return;
    m_status = s;
    emit statusChanged(m_status);
}

void ClientBridge::pair(const QString &code) {
    const QString normalised = code.trimmed().toUpper();
    QSettings s;
    s.setValue(QStringLiteral("admin/code"), normalised);
    s.setValue(QStringLiteral("admin/code_hash"), hashCode(normalised));
    setStatus(QStringLiteral("Searching for admin..."));
    emit pairedChanged();
    startDiscovery();
}

void ClientBridge::connectIfPaired() {
    if (isPaired()) {
        setStatus(QStringLiteral("Searching for admin..."));
        startDiscovery();
    }
}

void ClientBridge::sendMessage(const QString &text) {
    QJsonObject payload;
    payload["text"] = text;
    m_client->sendText(makeMessage(MSG_TEXT_MESSAGE_REPLY, payload));
}

void ClientBridge::sendSignalAck(const QString &color, const QString &text) {
    QJsonObject payload;
    payload["color"] = color;
    payload["text"]  = text;
    m_client->sendText(makeMessage(MSG_SIGNAL_ACK, payload));
}

void ClientBridge::onAdminFound(const QString &ip, quint16 port) {
    m_discovery->stop();
    QSettings s;
    const QString code     = s.value(QStringLiteral("admin/code")).toString();
    const QString hostname = QSysInfo::machineHostName();
    const QString username = qEnvironmentVariable(
        "USERNAME", qEnvironmentVariable("USER", QStringLiteral("student")));
    qInfo() << "[Client] Connecting to discovered admin at" << ip;
    m_client->connectToAdmin(ip, port, code, hostname, username);
}

void ClientBridge::onTextMessage(const QString &raw) {
    const QJsonObject msg = parseMessage(raw);
    const QString type = msg["type"].toString();

    if (type == QLatin1String(MSG_START_STREAM)) { m_capturer->start(); return; }
    if (type == QLatin1String(MSG_STOP_STREAM))  { m_capturer->stop();  return; }
    if (type == QLatin1String(MSG_AUDIO_START_CAPTURE)) { m_audioManager->startMicCapture(); return; }
    if (type == QLatin1String(MSG_AUDIO_STOP_CAPTURE))  { m_audioManager->stopMicCapture();  return; }
    if (type == QLatin1String(MSG_AUDIO_START_PLAY))    { m_audioManager->startPlayback();   return; }
    if (type == QLatin1String(MSG_AUDIO_STOP_PLAY))     { m_audioManager->stopPlayback();    return; }

    if (type == QLatin1String(MSG_SET_CAPTURE_INTERVAL)) {
        m_capturer->setCaptureInterval(msg["intervalMs"].toInt());
        return;
    }
    if (type == QLatin1String(MSG_SET_STREAM_QUALITY)) {
        m_capturer->setQuality(msg["maxWidth"].toInt(), msg["quality"].toInt());
        return;
    }
    if (type == QLatin1String(MSG_REQUEST_CLIPBOARD)) {
        m_client->sendText(makeClipboardTextMessage(QGuiApplication::clipboard()->text()));
        return;
    }
    if (type == QLatin1String(MSG_CLIPBOARD_TEXT)) {
        const QString text = msg["text"].toString();
        QGuiApplication::clipboard()->setText(text);
#ifdef Q_OS_WIN
        keybd_event(0x11, 0, 0, 0);
        keybd_event(0x56, 0, 0, 0);
        keybd_event(0x56, 0, KEYEVENTF_KEYUP, 0);
        keybd_event(0x11, 0, KEYEVENTF_KEYUP, 0);
#endif
        return;
    }
    if (type == QLatin1String(MSG_TEXT_MESSAGE)) {
        emit messageReceived(msg["text"].toString());
        return;
    }
    if (type == QLatin1String(MSG_SIGNAL)) {
        const QString color = msg["color"].toString();
        const QString text  = msg["text"].toString();
        emit signalReceived(color, text);
        sendSignalAck(color, text);
        return;
    }
    if (type == QLatin1String(MSG_RUN_COMMAND)) {
        const QString cmd = msg["cmd"].toString();
        if (!cmd.isEmpty()) {
            qInfo() << "[Client] Executing command:" << cmd;
#ifdef Q_OS_WIN
            QProcess::startDetached("cmd.exe", QStringList() << "/c" << cmd);
#else
            QProcess::startDetached("/bin/sh", QStringList() << "-c" << cmd);
#endif
        }
        return;
    }

#ifdef Q_OS_WIN
    if (type == QLatin1String(MSG_MOUSE_MOVE)) {
        const int x = msg["x"].toInt();
        const int y = msg["y"].toInt();
        if (msg["absolute"].toBool(true)) {
            SetCursorPos(x, y);
        } else {
            POINT p; GetCursorPos(&p);
            SetCursorPos(p.x + x, p.y + y);
        }
        return;
    }
    if (type == QLatin1String(MSG_MOUSE_CLICK)) {
        const int button  = msg["button"].toInt();
        const bool press  = msg["press"].toBool();
        const bool release = msg["release"].toBool();
        DWORD flags = 0;
        if (button == 1) {
            if (press)   { flags |= MOUSEEVENTF_LEFTDOWN; m_mouseLeftDown = true; }
            if (release) { flags |= MOUSEEVENTF_LEFTUP;   m_mouseLeftDown = false; }
        } else if (button == 2) {
            if (press)   { flags |= MOUSEEVENTF_RIGHTDOWN; m_mouseRightDown = true; }
            if (release) { flags |= MOUSEEVENTF_RIGHTUP;   m_mouseRightDown = false; }
        } else if (button == 3) {
            if (press)   { flags |= MOUSEEVENTF_MIDDLEDOWN; m_mouseMiddleDown = true; }
            if (release) { flags |= MOUSEEVENTF_MIDDLEUP;   m_mouseMiddleDown = false; }
        }
        if (flags) mouse_event(flags, 0, 0, 0, 0);
        return;
    }
    if (type == QLatin1String(MSG_MOUSE_SCROLL)) {
        mouse_event(MOUSEEVENTF_WHEEL, 0, 0, msg["delta"].toInt(), 0);
        return;
    }
    if (type == QLatin1String(MSG_KEYBOARD_KEY)) {
        const int keyCode  = msg["key"].toInt();
        if (msg["press"].toBool())   { keybd_event(keyCode, 0, 0, 0); m_pressedKeys.insert(keyCode); }
        if (msg["release"].toBool()) {
            if (m_pressedKeys.remove(keyCode))
                keybd_event(keyCode, 0, KEYEVENTF_KEYUP, 0);
        }
        return;
    }
    if (type == QLatin1String(MSG_KEYBOARD_TEXT)) {
        const QString text = msg["text"].toString();
        for (QChar ch : std::as_const(text)) {
            INPUT inputs[2] = {};
            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wScan = ch.unicode();
            inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wScan = ch.unicode();
            inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
            SendInput(2, inputs, sizeof(INPUT));
        }
        return;
    }
#endif

    qWarning() << "[Client] Unknown message type:" << type;
}

void ClientBridge::onBinaryMessage(const QByteArray &data) {
    if (data.isEmpty()) return;
    if (static_cast<quint8>(data[0]) == static_cast<quint8>(BIN_AUDIO_PLAYBACK))
        m_audioManager->onPlaybackData(data.mid(1));
}

void ClientBridge::startDiscovery() {
    const QString hash = QSettings().value(QStringLiteral("admin/code_hash")).toString();
    if (hash.isEmpty()) return;
    m_discovery->setExpectedHash(hash);
    if (!m_discovery->start())
        setStatus(QStringLiteral("Cannot listen on this network."));
}
