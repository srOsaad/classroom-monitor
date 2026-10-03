#pragma once
#include <QDebug>
#include <QHostAddress>
#include <QObject>
#include <QUdpSocket>
#include "discovery.h"

class DiscoveryListener : public QObject {
    Q_OBJECT
public:
    explicit DiscoveryListener(QObject *parent = nullptr)
        : QObject(parent), m_socket(new QUdpSocket(this))
    {
        connect(m_socket, &QUdpSocket::readyRead,
                this, &DiscoveryListener::onReadyRead);
    }

    void setExpectedHash(const QString &hash) { m_expectedHash = hash; }

    bool start() {
        m_listening = true;
        if (m_socket->state() == QAbstractSocket::BoundState)
            return true;
        if (!m_socket->bind(QHostAddress::AnyIPv4, DISCOVERY_PORT,
                            QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
            qCritical() << "[Discovery] Cannot bind UDP port" << DISCOVERY_PORT
                        << ":" << m_socket->errorString();
            return false;
        }
        return true;
    }

    void stop() { m_listening = false; }

signals:
    void adminFound(const QString &ip, quint16 port);

private slots:
    void onReadyRead() {
        while (m_socket->hasPendingDatagrams()) {
            QByteArray   data;
            QHostAddress sender;
            data.resize(static_cast<int>(m_socket->pendingDatagramSize()));
            m_socket->readDatagram(data.data(), data.size(), &sender);

            if (!m_listening) continue;

            QString hash;
            quint16 port = 0;
            if (!parseDiscoveryBeacon(data, hash, port)) continue;
            if (m_expectedHash.isEmpty() || hash != m_expectedHash) continue;

            qInfo() << "[Discovery] Found admin at" << sender.toString()
                    << "port" << port;
            emit adminFound(sender.toString(), port);
        }
    }

private:
    QUdpSocket *m_socket;
    QString     m_expectedHash;
    bool        m_listening = false;
};
