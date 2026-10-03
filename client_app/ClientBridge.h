#pragma once
#include <QObject>
#include <QSet>
#include <QSettings>
#include <QString>
#include <QSysInfo>
#include "ClientAudioManager.h"
#include "DiscoveryListener.h"
#include "HeartbeatService.h"
#include "ScreenCapturer.h"
#include "WebSocketClient.h"
#include "discovery.h"

class ClientBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool paired READ isPaired NOTIFY pairedChanged)

public:
    explicit ClientBridge(QObject *parent = nullptr);

    QString status() const { return m_status; }
    bool isPaired() const {
        return !QSettings().value(QStringLiteral("admin/code")).toString().isEmpty();
    }

    Q_INVOKABLE void pair(const QString &code);
    Q_INVOKABLE void connectIfPaired();
    Q_INVOKABLE void sendMessage(const QString &text);
    Q_INVOKABLE void sendSignalAck(const QString &color, const QString &text);

signals:
    void statusChanged(const QString &status);
    void pairedChanged();
    void messageReceived(const QString &text);
    void signalReceived(const QString &color, const QString &text);

private slots:
    void onAdminFound(const QString &ip, quint16 port);
    void onTextMessage(const QString &raw);
    void onBinaryMessage(const QByteArray &data);

private:
    void startDiscovery();
    void setStatus(const QString &s);

    QString             m_status;
    WebSocketClient    *m_client;
    HeartbeatService   *m_heartbeat;
    DiscoveryListener  *m_discovery;
    ScreenCapturer     *m_capturer;
    ClientAudioManager *m_audioManager;

    QSet<int> m_pressedKeys;
    bool m_mouseLeftDown        = false;
    bool m_mouseRightDown       = false;
    bool m_mouseMiddleDown      = false;
    bool m_remoteInputCleanupDone = false;
};
