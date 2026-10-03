#include "WebSocketClient.h"
#include "protocol.h"
#include <QDebug>
#include <QJsonObject>
#include <QUrl>

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , m_reconnectTimer(new QTimer(this))
{
    m_reconnectTimer->setSingleShot(true);

    connect(m_socket, &QWebSocket::connected,    this, &WebSocketClient::onConnected);
    connect(m_socket, &QWebSocket::disconnected, this, &WebSocketClient::onDisconnected);
    connect(m_socket, &QWebSocket::textMessageReceived,
            this, &WebSocketClient::onTextMessage);
    connect(m_socket, &QWebSocket::binaryMessageReceived,
            this, &WebSocketClient::onBinaryMessage);

    connect(m_socket, &QWebSocket::errorOccurred,
            this, &WebSocketClient::onErrorOccurred);
    connect(m_reconnectTimer, &QTimer::timeout,
            this, &WebSocketClient::attemptReconnect);
}

void WebSocketClient::connectToAdmin(const QString &ip, quint16 port,
                                     const QString &code,
                                     const QString &hostname,
                                     const QString &username) {
    m_ip       = ip;
    m_port     = port;
    m_code     = code;
    m_hostname = hostname;
    m_username = username;

    const QUrl url(QStringLiteral("ws://%1:%2").arg(ip).arg(port));
    qInfo() << "[Client] Connecting to" << url.toString();
    m_socket->open(url);
}

void WebSocketClient::sendText(const QByteArray &msg) {
    m_socket->sendTextMessage(QString::fromUtf8(msg));
}

void WebSocketClient::sendBinary(const QByteArray &data) {
    m_socket->sendBinaryMessage(data);
}

void WebSocketClient::onConnected() {
    qInfo() << "[Client] Connected, sending auth.";
    m_retryCount = 0;

    QJsonObject payload;
    payload["code"]     = m_code;
    payload["hostname"] = m_hostname;
    payload["username"] = m_username;
    sendText(makeMessage(MSG_AUTH, payload));
}

void WebSocketClient::onTextMessage(const QString &raw) {
    const QJsonObject msg = parseMessage(raw);
    const QString type = msg["type"].toString();

    if (type == QLatin1String(MSG_AUTH_OK)) {
        m_authenticated = true;
        qInfo() << "[Client] Auth accepted.";
        sendText(makeMessage(MSG_ONLINE));
        emit authenticated();
        return;
    }
    if (type == QLatin1String(MSG_AUTH_FAIL)) {
        qWarning() << "[Client] Auth rejected:" << msg["reason"].toString();
        emit authFailed(msg["reason"].toString());
        return;
    }
    if (type == QLatin1String(MSG_PONG)) {
        emit pongReceived();
        return;
    }
    emit textMessageReceived(raw);
}

void WebSocketClient::onBinaryMessage(const QByteArray &data) {
    emit binaryMessageReceived(data);
}

void WebSocketClient::onDisconnected() {
    m_authenticated = false;
    qWarning() << "[Client] Disconnected, will retry.";
    emit disconnectedFromAdmin();

    const int shift = qMin(m_retryCount, 4);
    const int delay = qMin(5000 * (1 << shift), 60000);
    m_retryCount++;
    m_reconnectTimer->start(delay);
    qInfo() << "[Client] Reconnecting in" << delay / 1000 << "seconds.";
}

void WebSocketClient::onErrorOccurred(QAbstractSocket::SocketError) {
    qWarning() << "[Client] Socket error:" << m_socket->errorString();
}

void WebSocketClient::attemptReconnect() {
    const QUrl url(QStringLiteral("ws://%1:%2").arg(m_ip).arg(m_port));
    m_socket->open(url);
}
