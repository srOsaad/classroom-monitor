#pragma once
#include <QByteArray>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

constexpr quint16 DISCOVERY_PORT        = 45454;
constexpr int     BROADCAST_INTERVAL_MS = 2000;
constexpr auto    DISCOVERY_APP_ID      = "ClassroomAdmin";

constexpr int MAX_BEACON_BYTES = 512;

inline QString hashCode(const QString &code) {
    return QString::fromLatin1(
        QCryptographicHash::hash(code.trimmed().toUpper().toUtf8(),
                                 QCryptographicHash::Sha256).toHex());
}

inline QByteArray makeDiscoveryBeacon(const QString &codeHash, quint16 wsPort) {
    QJsonObject obj;
    obj["app"]  = DISCOVERY_APP_ID;
    obj["hash"] = codeHash;
    obj["port"] = static_cast<int>(wsPort);
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

inline bool parseDiscoveryBeacon(const QByteArray &data,
                                 QString          &outHash,
                                 quint16          &outPort) {
    if (data.isEmpty() || data.size() > MAX_BEACON_BYTES) return false;

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return false;

    const QJsonObject obj = doc.object();
    if (obj["app"].toString() != QLatin1String(DISCOVERY_APP_ID)) return false;

    const QString hash = obj["hash"].toString();
    const int     port = obj["port"].toInt();
    if (hash.isEmpty() || port <= 0 || port > 65535) return false;

    outHash = hash;
    outPort = static_cast<quint16>(port);
    return true;
}
