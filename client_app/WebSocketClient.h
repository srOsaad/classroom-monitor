#pragma once
#include <QAbstractSocket>
#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QWebSocket>

class WebSocketClient : public QObject {
    Q_OBJECT
public:
    explicit WebSocketClient(QObject *parent = nullptr);

    void connectToAdmin(const QString &ip, quint16 port,
                        const QString &code,
                        const QString &hostname,
                        const QString &username);

    void sendText(const QByteArray &msg);
    void sendBinary(const QByteArray &data);

    bool isAuthenticated() const { return m_authenticated; }

signals:
    void authenticated();
    void authFailed(const QString &reason);
    void disconnectedFromAdmin();
    void pongReceived();
    void textMessageReceived(const QString &msg);
    void binaryMessageReceived(const QByteArray &data);

private slots:
    void onConnected();
    void onTextMessage(const QString &raw);
    void onBinaryMessage(const QByteArray &data);
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError error);
    void attemptReconnect();

private:
    QWebSocket *m_socket;
    QTimer     *m_reconnectTimer;

    QString m_ip;
    quint16 m_port          = 9000;
    QString m_code;
    QString m_hostname;
    QString m_username;
    bool    m_authenticated = false;
    int     m_retryCount    = 0;
};
