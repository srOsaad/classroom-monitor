#pragma once
#include <QObject>
#include <QTimer>
#include "WebSocketClient.h"
#include "protocol.h"

class HeartbeatService : public QObject {
    Q_OBJECT
public:
    static constexpr int DEFAULT_INTERVAL_MS = 5000;

    explicit HeartbeatService(WebSocketClient *client,
                              int intervalMs = DEFAULT_INTERVAL_MS,
                              QObject *parent = nullptr)
        : QObject(parent)
        , m_client(client)
        , m_timer(new QTimer(this))
    {
        m_timer->setInterval(intervalMs);
        connect(m_timer, &QTimer::timeout, this, &HeartbeatService::sendPing);
    }

    void start() { m_timer->start(); }
    void stop() { m_timer->stop(); }

private slots:
    void sendPing() {
        if (m_client->isAuthenticated())
            m_client->sendText(makeMessage(MSG_PING));
    }

private:
    WebSocketClient *m_client;
    QTimer          *m_timer;
};
