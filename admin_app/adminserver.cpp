#include "adminserver.h"
#include "PairingManager.h"
#include "protocol.h"
#include <QClipboard>
#include <QDebug>
#include <QGuiApplication>
#include <QJsonObject>

AdminServer::AdminServer(quint16 port, QObject *parent)
    : QObject(parent)
    , m_server(new QWebSocketServer(QStringLiteral("ClassroomAdmin"),
                                    QWebSocketServer::NonSecureMode, this))
    , m_registry(new ClientRegistry(this))
    , m_streamManager(new ScreenStreamManager(m_registry, this))
    , m_audioManager(new AdminAudioManager(this))
{
    if (m_server->listen(QHostAddress::Any, port)) {
        qInfo() << "[Admin] Server listening on port" << port;
        qInfo() << "[Admin] Pairing code:" << PairingManager::savedCode();
    } else {
        qCritical() << "[Admin] Failed to listen:" << m_server->errorString();
    }

    connect(m_server, &QWebSocketServer::newConnection, this, &AdminServer::onNewConnection);
    connect(m_registry, &ClientRegistry::clientListChanged, this, &AdminServer::clientCountChanged);
    connect(this, &AdminServer::screenFrameReceived, m_streamManager, &ScreenStreamManager::onFrameReceived);

    connect(m_audioManager, &AdminAudioManager::audioPlaybackDataReady,
            this, [](QWebSocket *target, const QByteArray &data) {
                QByteArray msg;
                msg.append(BIN_AUDIO_PLAYBACK);
                msg.append(data);
                target->sendBinaryMessage(msg);
            });
}

AdminServer::~AdminServer() {
    m_server->close();
    for (QWebSocket *socket : m_registry->allSockets())
        socket->abort();
}

bool AdminServer::isListening() const { return m_server->isListening(); }

QString AdminServer::pairingCode() const { return PairingManager::savedCode(); }

void AdminServer::regenerateCode() {
    const QString code = PairingManager::generateCode();
    qInfo() << "[Admin] Pairing code regenerated:" << code;
    emit pairingCodeChanged();

}

QWebSocket *AdminServer::findSocketByHostname(const QString &hostname) const {
    for (QWebSocket *sock : m_registry->allSockets()) {
        if (m_registry->get(sock).hostname == hostname)
            return sock;
    }
    return nullptr;
}

void AdminServer::onNewConnection() {
    QWebSocket *socket = m_server->nextPendingConnection();
    if (!socket) return;
    qInfo() << "[Admin] New connection from" << socket->peerAddress().toString();

    ClientInfo info;
    info.connectedAt = QDateTime::currentDateTime();
    info.address     = socket->peerAddress().toString();
    m_registry->add(socket, info);

    connect(socket, &QWebSocket::textMessageReceived, this, &AdminServer::onTextMessage);
    connect(socket, &QWebSocket::binaryMessageReceived, this, &AdminServer::onBinaryMessage);
    connect(socket, &QWebSocket::disconnected, this, &AdminServer::onClientDisconnected);
}

void AdminServer::onTextMessage(const QString &raw) {
    QWebSocket *socket = qobject_cast<QWebSocket *>(sender());
    if (!socket || !m_registry->contains(socket)) return;

    const QJsonObject msg = parseMessage(raw);
    const QString type = msg["type"].toString();

    if (type == QLatin1String(MSG_AUTH)) {
        const QString code     = msg["code"].toString();
        const QString rawHost  = msg["hostname"].toString();
        const QString username = msg["username"].toString();

        if (PairingManager::isValid(code)) {
            ClientInfo &info   = m_registry->get(socket);
            info.authenticated = true;

            const QString base = rawHost.isEmpty() ? socket->peerAddress().toString()
                                                   : rawHost;
            info.hostname      = m_registry->uniqueHostname(base, socket);
            info.username      = username;
            info.lastSeen      = QDateTime::currentDateTime();

            socket->sendTextMessage(QString::fromUtf8(makeMessage(MSG_AUTH_OK)));
            qInfo() << "[Admin] Authenticated:" << info.hostname << username;
            emit clientAuthenticated(info.hostname, username);

            m_streamManager->addStudent(socket);
            info.streaming = true;
            socket->sendTextMessage(QString::fromUtf8(makeMessage(MSG_START_STREAM)));
            emit clientCountChanged();
        } else {
            socket->sendTextMessage(QString::fromUtf8(
                makeMessage(MSG_AUTH_FAIL, {{"reason", QStringLiteral("Invalid pairing code")}})));
            qWarning() << "[Admin] Auth rejected from" << socket->peerAddress().toString();
            socket->close();
        }
        return;
    }

    if (!m_registry->get(socket).authenticated) {
        socket->close();
        return;
    }

    if (type == QLatin1String(MSG_PING)) {
        m_registry->get(socket).lastSeen = QDateTime::currentDateTime();
        socket->sendTextMessage(QString::fromUtf8(makeMessage(MSG_PONG)));
        emit heartbeatReceived(m_registry->get(socket).hostname);
        emit clientCountChanged();
        return;
    }

    if (type == QLatin1String(MSG_ONLINE)) {
        qInfo() << "[Admin]" << m_registry->get(socket).hostname << "is online";
        return;
    }

    if (type == QLatin1String(MSG_TEXT_MESSAGE_REPLY)) {
        emit messageReceived(m_registry->get(socket).hostname, msg["text"].toString());
        return;
    }

    if (type == QLatin1String(MSG_CLIPBOARD_TEXT)) {
        QGuiApplication::clipboard()->setText(msg["text"].toString());
        qInfo() << "[Admin] Received clipboard text from client";
        return;
    }

    if (type == QLatin1String(MSG_SIGNAL_ACK)) {
        emit signalReceived(m_registry->get(socket).hostname,
                            msg["color"].toString(), msg["text"].toString());
        return;
    }

    if (type == QLatin1String(MSG_SCREEN_SIZE)) {
        ClientInfo &info  = m_registry->get(socket);
        info.screenWidth  = msg["width"].toInt();
        info.screenHeight = msg["height"].toInt();
        qInfo() << "[Admin] Screen size for" << info.hostname
                << ":" << info.screenWidth << "x" << info.screenHeight;
        return;
    }

    qWarning() << "[Admin] Unknown message type:" << type;
}

void AdminServer::onBinaryMessage(const QByteArray &data) {
    QWebSocket *socket = qobject_cast<QWebSocket *>(sender());
    if (!socket || !m_registry->contains(socket)) return;
    if (!m_registry->get(socket).authenticated || data.isEmpty()) return;

    const quint8 type = static_cast<quint8>(data[0]);
    if (type == static_cast<quint8>(BIN_SCREEN_FRAME)) {
        emit screenFrameReceived(socket, data.mid(1));
    } else if (type == static_cast<quint8>(BIN_AUDIO_CAPTURE)) {
        m_audioManager->onAudioDataReceived(socket, data.mid(1));
    } else if (type == static_cast<quint8>(BIN_CAMERA_FRAME)) {
        m_streamManager->onCameraFrameReceived(socket, data.mid(1));
    }
}

void AdminServer::onClientDisconnected() {
    QWebSocket *socket = qobject_cast<QWebSocket *>(sender());
    if (!socket) return;

    const QString hostname = m_registry->get(socket).hostname;
    qInfo() << "[Admin] Disconnected:" << hostname;

    m_streamManager->removeStudent(socket);
    m_registry->remove(socket);
    socket->deleteLater();
    emit clientDisconnected(hostname);
}

void AdminServer::startAllStreams() {
    const QByteArray msg = makeMessage(MSG_START_STREAM);
    for (QWebSocket *sock : m_registry->allSockets()) {
        ClientInfo &info = m_registry->get(sock);
        if (!info.authenticated) continue;
        info.streaming = true;
        sock->sendTextMessage(QString::fromUtf8(msg));
    }
    emit clientCountChanged();
}

void AdminServer::stopAllStreams() {
    const QByteArray msg = makeMessage(MSG_STOP_STREAM);
    for (QWebSocket *sock : m_registry->allSockets()) {
        ClientInfo &info = m_registry->get(sock);
        if (!info.authenticated) continue;
        info.streaming = false;
        sock->sendTextMessage(QString::fromUtf8(msg));
    }
    emit clientCountChanged();
}

void AdminServer::startCameraStream(const QString &hostname) {
    if (QWebSocket *sock = findSocketByHostname(hostname))
        sock->sendTextMessage(QString::fromUtf8(makeMessage(MSG_CAMERA_START)));
}

void AdminServer::stopCameraStream(const QString &hostname) {
    if (QWebSocket *sock = findSocketByHostname(hostname))
        sock->sendTextMessage(QString::fromUtf8(makeMessage(MSG_CAMERA_STOP)));
}

void AdminServer::requestHighQualityStream(const QString &hostname, bool highQuality) {
    QWebSocket *sock = findSocketByHostname(hostname);
    if (!sock) return;
    const int maxWidth = highQuality ? 1920 : 1280;
    const int quality  = highQuality ? 70 : 40;
    sock->sendTextMessage(QString::fromUtf8(makeSetQualityMessage(maxWidth, quality)));
}

void AdminServer::setCaptureInterval(const QString &hostname, int intervalMs) {
    if (QWebSocket *sock = findSocketByHostname(hostname))
        sock->sendTextMessage(QString::fromUtf8(makeSetCaptureIntervalMessage(intervalMs)));
}

void AdminServer::sendMessageTo(const QString &targetHostname, const QString &text) {
    if (QWebSocket *sock = findSocketByHostname(targetHostname))
        sock->sendTextMessage(QString::fromUtf8(makeTextMessage(targetHostname, text)));
}

void AdminServer::sendMessageToAll(const QString &text) {
    const QByteArray msg = makeTextMessage(QStringLiteral("all"), text);
    for (QWebSocket *sock : m_registry->allSockets())
        if (m_registry->get(sock).authenticated)
            sock->sendTextMessage(QString::fromUtf8(msg));
}

void AdminServer::sendSignalTo(const QString &targetHostname, const QString &color, const QString &text) {
    if (QWebSocket *sock = findSocketByHostname(targetHostname))
        sock->sendTextMessage(QString::fromUtf8(makeSignalMessage(color, text)));
}

void AdminServer::sendSignalToAll(const QString &color, const QString &text) {
    const QByteArray msg = makeSignalMessage(color, text);
    for (QWebSocket *sock : m_registry->allSockets())
        if (m_registry->get(sock).authenticated)
            sock->sendTextMessage(QString::fromUtf8(msg));
}

void AdminServer::startListeningTo(const QString &hostname) {
    QWebSocket *sock = findSocketByHostname(hostname);
    if (!sock) return;
    sock->sendTextMessage(QString::fromUtf8(makeMessage(MSG_AUDIO_START_CAPTURE)));
    m_audioManager->startListeningTo(sock);
}

void AdminServer::stopListening() {
    if (m_audioManager->currentListeningSocket())
        m_audioManager->currentListeningSocket()->sendTextMessage(
            QString::fromUtf8(makeMessage(MSG_AUDIO_STOP_CAPTURE)));
    m_audioManager->stopListening();
}

void AdminServer::startSpeakingTo(const QString &hostname) {
    QWebSocket *sock = findSocketByHostname(hostname);
    if (!sock) return;
    sock->sendTextMessage(QString::fromUtf8(makeMessage(MSG_AUDIO_START_PLAY)));
    m_audioManager->startSpeakingTo(sock);
}

void AdminServer::stopSpeaking() {
    if (m_audioManager->currentSpeakingSocket())
        m_audioManager->currentSpeakingSocket()->sendTextMessage(
            QString::fromUtf8(makeMessage(MSG_AUDIO_STOP_PLAY)));
    m_audioManager->stopSpeaking();
}

void AdminServer::startSpeakingToAll() {
    QList<QWebSocket *> authSockets;
    for (QWebSocket *sock : m_registry->allSockets()) {
        if (m_registry->get(sock).authenticated) {
            sock->sendTextMessage(QString::fromUtf8(makeMessage(MSG_AUDIO_START_PLAY)));
            authSockets.append(sock);
        }
    }
    m_audioManager->startSpeakingToAll(authSockets);
}

void AdminServer::stopSpeakingToAll() {
    for (QWebSocket *sock : m_registry->allSockets())
        if (m_registry->get(sock).authenticated)
            sock->sendTextMessage(QString::fromUtf8(makeMessage(MSG_AUDIO_STOP_PLAY)));
    m_audioManager->stopSpeakingToAll();
}

void AdminServer::runCommandOn(const QString &hostname, const QString &command) {
    QWebSocket *sock = findSocketByHostname(hostname);
    if (!sock) return;
    QJsonObject payload;
    payload["cmd"] = command;
    sock->sendTextMessage(QString::fromUtf8(makeMessage(MSG_RUN_COMMAND, payload)));
}

void AdminServer::runCommandOnAll(const QString &command) {
    QJsonObject payload;
    payload["cmd"] = command;
    const QByteArray msg = makeMessage(MSG_RUN_COMMAND, payload);
    for (QWebSocket *sock : m_registry->allSockets())
        if (m_registry->get(sock).authenticated)
            sock->sendTextMessage(QString::fromUtf8(msg));
}

void AdminServer::requestClipboard(const QString &hostname) {
    if (QWebSocket *sock = findSocketByHostname(hostname))
        sock->sendTextMessage(QString::fromUtf8(makeMessage(MSG_REQUEST_CLIPBOARD)));
}

void AdminServer::sendClipboardText(const QString &hostname, const QString &text) {
    if (QWebSocket *sock = findSocketByHostname(hostname))
        sock->sendTextMessage(QString::fromUtf8(makeClipboardTextMessage(text)));
}

QString AdminServer::getClipboardText() const {
    return QGuiApplication::clipboard()->text();
}

void AdminServer::sendMouseEventNormalized(const QString &hostname, const QString &action,
                                           double normX, double normY, int button) {
    QWebSocket *sock = findSocketByHostname(hostname);
    if (!sock) return;

    const ClientInfo &info = m_registry->get(sock);
    const int clientW = info.screenWidth;
    const int clientH = info.screenHeight;
    if (clientW <= 0 || clientH <= 0) return;

    normX = qBound(0.0, normX, 1.0);
    normY = qBound(0.0, normY, 1.0);
    const int absX = qBound(0, int(normX * clientW), clientW - 1);
    const int absY = qBound(0, int(normY * clientH), clientH - 1);

    if (action == QLatin1String("move")) {
        sendMouseMove(hostname, absX, absY, true);
    } else if (action == QLatin1String("press")) {
        sendMouseMove(hostname, absX, absY, true);
        sendMouseClick(hostname, button, true, false);
    } else if (action == QLatin1String("release")) {
        sendMouseClick(hostname, button, false, true);
    } else if (action == QLatin1String("click")) {
        sendMouseMove(hostname, absX, absY, true);
        sendMouseClick(hostname, button, true, true);
    } else if (action == QLatin1String("doubleclick")) {
        sendMouseMove(hostname, absX, absY, true);
        sendMouseClick(hostname, button, true, true);
        sendMouseClick(hostname, button, true, true);
    }
}

void AdminServer::sendMouseMove(const QString &hostname, int x, int y, bool absolute) {
    if (QWebSocket *sock = findSocketByHostname(hostname))
        sock->sendTextMessage(QString::fromUtf8(makeMouseMoveMessage(x, y, absolute)));
}

void AdminServer::sendMouseClick(const QString &hostname, int button, bool press, bool release) {
    if (QWebSocket *sock = findSocketByHostname(hostname))
        sock->sendTextMessage(QString::fromUtf8(makeMouseClickMessage(button, press, release)));
}

void AdminServer::sendMouseScroll(const QString &hostname, int delta) {
    if (QWebSocket *sock = findSocketByHostname(hostname))
        sock->sendTextMessage(QString::fromUtf8(makeMouseScrollMessage(delta)));
}

void AdminServer::sendKeyPress(const QString &hostname, int keyCode) {
    if (QWebSocket *sock = findSocketByHostname(hostname))
        sock->sendTextMessage(QString::fromUtf8(makeKeyboardKeyMessage(keyCode, true, false)));
}

void AdminServer::sendKeyRelease(const QString &hostname, int keyCode) {
    if (QWebSocket *sock = findSocketByHostname(hostname))
        sock->sendTextMessage(QString::fromUtf8(makeKeyboardKeyMessage(keyCode, false, true)));
}

void AdminServer::sendKeySequence(const QString &hostname, const QString &text) {
    if (QWebSocket *sock = findSocketByHostname(hostname))
        sock->sendTextMessage(QString::fromUtf8(makeKeyboardTextMessage(text)));
}

void AdminServer::sendKeyTap(const QString &hostname, int keyCode) {
    sendKeyPress(hostname, keyCode);
    sendKeyRelease(hostname, keyCode);
}

void AdminServer::releaseAllInputs(const QString &hostname) {
    if (!findSocketByHostname(hostname)) return;
    sendMouseClick(hostname, 1, false, true);
    sendKeyRelease(hostname, 0x11);
    sendKeyRelease(hostname, 0x12);
    sendKeyRelease(hostname, 0x10);
}
