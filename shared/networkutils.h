#pragma once
#include <QHostAddress>
#include <QList>
#include <QNetworkInterface>
#include <QString>

inline bool isUsableInterface(const QNetworkInterface &iface) {
    const auto flags = iface.flags();
    return flags.testFlag(QNetworkInterface::IsUp)
        && flags.testFlag(QNetworkInterface::IsRunning)
        && !flags.testFlag(QNetworkInterface::IsLoopBack);
}

inline QString localIPv4() {
    QString linkLocalFallback;

    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        if (!isUsableInterface(iface)) continue;

        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            const QHostAddress ip = entry.ip();
            if (ip.protocol() != QAbstractSocket::IPv4Protocol) continue;

            if (ip.isLinkLocal()) {
                if (linkLocalFallback.isEmpty())
                    linkLocalFallback = ip.toString();
                continue;
            }
            return ip.toString();
        }
    }

    return linkLocalFallback.isEmpty() ? QStringLiteral("127.0.0.1") : linkLocalFallback;
}

inline QList<QHostAddress> broadcastAddresses() {
    QList<QHostAddress> result;

    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        if (!isUsableInterface(iface)) continue;
        if (!iface.flags().testFlag(QNetworkInterface::CanBroadcast)) continue;

        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol) continue;
            const QHostAddress bcast = entry.broadcast();
            if (!bcast.isNull() && !result.contains(bcast))
                result.append(bcast);
        }
    }

    if (result.isEmpty())
        result.append(QHostAddress::Broadcast);
    return result;
}
