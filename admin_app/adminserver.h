#pragma once
#include <QObject>
#include <QVariantList>
#include <QWebSocket>
#include <QWebSocketServer>
#include "AdminAudioManager.h"
#include "ClientRegistry.h"
#include "ScreenStreamManager.h"

class AdminServer : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool listening READ isListening NOTIFY listeningChanged)
    Q_PROPERTY(int studentCount READ studentCount NOTIFY clientCountChanged)
    Q_PROPERTY(QString pairingCode READ pairingCode NOTIFY pairingCodeChanged)
    Q_PROPERTY(QVariantList students READ students NOTIFY clientCountChanged)

public:
    explicit AdminServer(quint16 port = 9000, QObject *parent = nullptr);
    ~AdminServer() override;

    bool isListening() const;
    ClientRegistry      *registry()      { return m_registry; }
    ScreenStreamManager *streamManager() { return m_streamManager; }

    int studentCount() const { return m_registry ? m_registry->authenticatedCount() : 0; }
    QString pairingCode() const;
    QVariantList students() const { return m_registry ? m_registry->students() : QVariantList(); }

    Q_INVOKABLE void regenerateCode();

    Q_INVOKABLE void setCaptureInterval(const QString &hostname, int intervalMs);
    Q_INVOKABLE void requestHighQualityStream(const QString &hostname, bool highQuality);
    Q_INVOKABLE void startAllStreams();
    Q_INVOKABLE void stopAllStreams();
    Q_INVOKABLE void startCameraStream(const QString &hostname);
    Q_INVOKABLE void stopCameraStream(const QString &hostname);

    Q_INVOKABLE void sendMessageTo(const QString &targetHostname, const QString &text);
    Q_INVOKABLE void sendMessageToAll(const QString &text);
    Q_INVOKABLE void sendSignalTo(const QString &targetHostname, const QString &color, const QString &text = "");
    Q_INVOKABLE void sendSignalToAll(const QString &color, const QString &text = "");

    Q_INVOKABLE void startListeningTo(const QString &hostname);
    Q_INVOKABLE void stopListening();
    Q_INVOKABLE void startSpeakingTo(const QString &hostname);
    Q_INVOKABLE void stopSpeaking();
    Q_INVOKABLE void startSpeakingToAll();
    Q_INVOKABLE void stopSpeakingToAll();

    Q_INVOKABLE void runCommandOn(const QString &hostname, const QString &command);
    Q_INVOKABLE void runCommandOnAll(const QString &command);

    Q_INVOKABLE void requestClipboard(const QString &hostname);
    Q_INVOKABLE void sendClipboardText(const QString &hostname, const QString &text);
    Q_INVOKABLE QString getClipboardText() const;

    Q_INVOKABLE void sendMouseEventNormalized(const QString &hostname, const QString &action,
                                              double normX, double normY, int button = 1);
    Q_INVOKABLE void sendMouseMove(const QString &hostname, int x, int y, bool absolute = true);
    Q_INVOKABLE void sendMouseClick(const QString &hostname, int button, bool press, bool release);
    Q_INVOKABLE void sendMouseScroll(const QString &hostname, int delta);
    Q_INVOKABLE void sendKeyPress(const QString &hostname, int keyCode);
    Q_INVOKABLE void sendKeyRelease(const QString &hostname, int keyCode);
    Q_INVOKABLE void sendKeySequence(const QString &hostname, const QString &text);
    Q_INVOKABLE void sendKeyTap(const QString &hostname, int keyCode);
    Q_INVOKABLE void releaseAllInputs(const QString &hostname);

signals:
    void listeningChanged();
    void clientCountChanged();
    void pairingCodeChanged();
    void clientAuthenticated(const QString &hostname, const QString &username);
    void clientDisconnected(const QString &hostname);
    void heartbeatReceived(const QString &hostname);
    void screenFrameReceived(QWebSocket *socket, const QByteArray &frame);
    void messageReceived(const QString &fromHostname, const QString &text);
    void signalReceived(const QString &fromHostname, const QString &color, const QString &text);

private slots:
    void onNewConnection();
    void onTextMessage(const QString &message);
    void onBinaryMessage(const QByteArray &data);
    void onClientDisconnected();

private:
    QWebSocket *findSocketByHostname(const QString &hostname) const;

    QWebSocketServer    *m_server;
    ClientRegistry      *m_registry;
    ScreenStreamManager *m_streamManager;
    AdminAudioManager   *m_audioManager;
};
