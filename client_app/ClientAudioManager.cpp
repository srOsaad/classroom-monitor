#include "ClientAudioManager.h"
#include "protocol.h"
#include <QDebug>

ClientAudioManager::ClientAudioManager(WebSocketClient *client, QObject *parent)
    : QObject(parent), m_client(client)
{
    m_format.setSampleRate(16000);
    m_format.setChannelCount(1);
    m_format.setSampleFormat(QAudioFormat::Int16);
}

ClientAudioManager::~ClientAudioManager() {
    stopMicCapture();
    stopPlayback();
}

void ClientAudioManager::startMicCapture() {
    if (m_micSource) stopMicCapture();
    const QAudioDevice inputDev = QMediaDevices::defaultAudioInput();
    if (!inputDev.isFormatSupported(m_format)) {
        qWarning() << "[ClientAudio] Mic format not supported";
        return;
    }
    m_micSource = new QAudioSource(inputDev, m_format, this);
    m_micDevice = m_micSource->start();
    connect(m_micDevice, &QIODevice::readyRead, this, &ClientAudioManager::onMicReadyRead);
    qInfo() << "[ClientAudio] Mic capture started";
}

void ClientAudioManager::stopMicCapture() {
    if (m_micSource) {
        m_micSource->stop();
        delete m_micSource;
        m_micSource = nullptr;
    }
    m_micDevice = nullptr;
}

void ClientAudioManager::startPlayback() {
    if (m_playSink) stopPlayback();
    const QAudioDevice outputDev = QMediaDevices::defaultAudioOutput();
    if (!outputDev.isFormatSupported(m_format)) {
        qWarning() << "[ClientAudio] Playback format not supported";
        return;
    }
    m_playSink   = new QAudioSink(outputDev, m_format, this);
    m_playDevice = m_playSink->start();
}

void ClientAudioManager::stopPlayback() {
    if (m_playSink) {
        m_playSink->stop();
        delete m_playSink;
        m_playSink = nullptr;
    }
    m_playDevice = nullptr;
}

void ClientAudioManager::onMicReadyRead() {
    if (!m_micDevice) return;
    const QByteArray data = m_micDevice->readAll();
    if (data.isEmpty()) return;
    QByteArray message;
    message.append(BIN_AUDIO_CAPTURE);
    message.append(data);
    m_client->sendBinary(message);
}

void ClientAudioManager::onPlaybackData(const QByteArray &pcmData) {
    if (m_playDevice) m_playDevice->write(pcmData);
}
