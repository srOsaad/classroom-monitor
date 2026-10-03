#pragma once
#include <QDebug>
#include <QHostAddress>
#include <QList>
#include <QObject>
#include <QTimer>
#include <QUdpSocket>
#include "PairingManager.h"
#include "discovery.h"
#include "networkutils.h"

class DiscoveryBroadcaster : public QObject {
    Q_OBJECT
public:
    explicit DiscoveryBroadcaster(quint16 wsPort = 9000, QObject *parent = nullptr)
        : QObject(parent), m_socket(new QUdpSocket(this)), m_timer(new QTimer(this)), m_wsPort(wsPort)
    {
        m_timer->setInterval(BROADCAST_INTERVAL_MS);
        connect(m_timer, &QTimer::timeout, this, &DiscoveryBroadcaster::sendBeacon);
    }

    void start() {
        m_timer->start();
        sendBeacon();
        qInfo() << "[Discovery] Broadcasting on UDP port" << DISCOVERY_PORT;
    }

    void stop() { m_timer->stop(); }

private slots:
    void sendBeacon() {
        const QByteArray beacon =
            makeDiscoveryBeacon(hashCode(PairingManager::savedCode()), m_wsPort);

        const QList<QHostAddress> targets = broadcastAddresses();
        for (const QHostAddress &target : targets)
            m_socket->writeDatagram(beacon, target, DISCOVERY_PORT);
    }

private:
    QUdpSocket *m_socket;
    QTimer     *m_timer;
    quint16     m_wsPort;
};
