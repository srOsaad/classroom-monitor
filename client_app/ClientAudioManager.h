#pragma once
#include <QAudioFormat>
#include <QAudioSink>
#include <QAudioSource>
#include <QIODevice>
#include <QMediaDevices>
#include <QObject>
#include "WebSocketClient.h"

class ClientAudioManager : public QObject {
    Q_OBJECT
public:
    explicit ClientAudioManager(WebSocketClient *client, QObject *parent = nullptr);
    ~ClientAudioManager() override;

public slots:
    void startMicCapture();
    void stopMicCapture();
    void startPlayback();
    void stopPlayback();
    void onPlaybackData(const QByteArray &pcmData);

private slots:
    void onMicReadyRead();

private:
    WebSocketClient *m_client;
    QAudioSource *m_micSource  = nullptr;
    QAudioSink   *m_playSink   = nullptr;
    QIODevice    *m_micDevice  = nullptr;
    QIODevice    *m_playDevice = nullptr;
    QAudioFormat  m_format;
};
