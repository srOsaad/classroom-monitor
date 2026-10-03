#pragma once
#include <QAudioFormat>
#include <QAudioSink>
#include <QAudioSource>
#include <QIODevice>
#include <QList>
#include <QMediaDevices>
#include <QObject>
#include <QWebSocket>

class AdminAudioManager : public QObject {
    Q_OBJECT
public:
    explicit AdminAudioManager(QObject *parent = nullptr);
    ~AdminAudioManager() override;

    void onAudioDataReceived(QWebSocket *studentSocket, const QByteArray &pcmData);
    void startListeningTo(QWebSocket *studentSocket);
    void stopListening();
    void startSpeakingTo(QWebSocket *targetSocket);
    void stopSpeaking();
    void startSpeakingToAll(const QList<QWebSocket *> &sockets);
    void stopSpeakingToAll();

    QWebSocket *currentListeningSocket() const { return m_currentListeningSocket; }
    QWebSocket *currentSpeakingSocket() const { return m_currentSpeakingSocket; }

signals:
    void audioPlaybackDataReady(QWebSocket *target, const QByteArray &pcmData);

private slots:
    void onMicrophoneReadyRead();

private:
    QAudioSource *m_micSource  = nullptr;
    QAudioSink   *m_playSink   = nullptr;
    QIODevice    *m_micDevice  = nullptr;
    QIODevice    *m_playDevice = nullptr;
    QWebSocket   *m_currentListeningSocket = nullptr;
    QWebSocket   *m_currentSpeakingSocket  = nullptr;
    QList<QWebSocket *> m_broadcastSockets;
    QAudioFormat  m_format;
};
