#pragma once
#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QWebSocket>

struct ClientInfo {
    QString   hostname;
    QString   username;
    QString   address;
    QDateTime connectedAt;
    QDateTime lastSeen;
    bool      authenticated = false;
    bool      streaming     = false;
    int       screenWidth   = 1920;
    int       screenHeight  = 1080;
};

class ClientRegistry : public QObject {
    Q_OBJECT
public:
    explicit ClientRegistry(QObject *parent = nullptr) : QObject(parent) {}

    void add(QWebSocket *socket, const ClientInfo &info) {
        m_clients[socket] = info;
        emit clientListChanged();
    }

    void remove(QWebSocket *socket) {
        if (m_clients.remove(socket) > 0)
            emit clientListChanged();
    }

    bool contains(QWebSocket *socket) const { return m_clients.contains(socket); }

    ClientInfo &get(QWebSocket *socket) { return m_clients[socket]; }

    QList<QWebSocket *> allSockets() const { return m_clients.keys(); }

    int count() const { return static_cast<int>(m_clients.size()); }

    int authenticatedCount() const {
        int n = 0;
        for (const ClientInfo &info : m_clients)
            if (info.authenticated) ++n;
        return n;
    }

    QString uniqueHostname(const QString &base, QWebSocket *except = nullptr) const {
        const auto taken = [&](const QString &name) {
            for (auto it = m_clients.constBegin(); it != m_clients.constEnd(); ++it) {
                if (it.key() == except) continue;
                if (it.value().hostname == name) return true;
            }
            return false;
        };
        if (!taken(base)) return base;
        for (int i = 2; ; ++i) {
            const QString candidate = QStringLiteral("%1 (%2)").arg(base).arg(i);
            if (!taken(candidate)) return candidate;
        }
    }

    QVariantList students() const {
        const QDateTime now = QDateTime::currentDateTime();
        QVariantList list;
        for (auto it = m_clients.constBegin(); it != m_clients.constEnd(); ++it) {
            const ClientInfo &info = it.value();
            if (!info.authenticated) continue;

            QVariantMap row;
            row["hostname"]  = info.hostname.isEmpty()
                                   ? it.key()->peerAddress().toString()
                                   : info.hostname;
            row["username"]  = info.username;
            row["address"]   = info.address;
            row["streaming"] = info.streaming;
            row["connectedAt"] = info.connectedAt.toString("hh:mm:ss");
            row["sessionSeconds"] = info.connectedAt.isValid()
                                        ? static_cast<int>(info.connectedAt.secsTo(now))
                                        : 0;
            row["idleSeconds"] = info.lastSeen.isValid()
                                     ? static_cast<int>(info.lastSeen.secsTo(now))
                                     : -1;
            list.append(row);
        }
        return list;
    }

signals:
    void clientListChanged();

private:
    QMap<QWebSocket *, ClientInfo> m_clients;
};
