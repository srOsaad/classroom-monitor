#include "AdminAudioManager.h"
#include <QDebug>

AdminAudioManager::AdminAudioManager(QObject *parent) : QObject(parent) {
    m_format.setSampleRate(16000);
    m_format.setChannelCount(1);
    m_format.setSampleFormat(QAudioFormat::Int16);
}

AdminAudioManager::~AdminAudioManager() {
    stopListening();
    stopSpeaking();
}

void AdminAudioManager::onAudioDataReceived(QWebSocket *studentSocket, const QByteArray &pcmData) {
    if (m_currentListeningSocket == studentSocket && m_playDevice)
        m_playDevice->write(pcmData);
}

void AdminAudioManager::startListeningTo(QWebSocket *studentSocket) {
    stopListening();
    m_currentListeningSocket = studentSocket;

    const QAudioDevice outputDevice = QMediaDevices::defaultAudioOutput();
    if (!outputDevice.isFormatSupported(m_format)) {
        qWarning() << "[AdminAudio] Output format not supported";
        return;
    }
    m_playSink   = new QAudioSink(outputDevice, m_format, this);
    m_playDevice = m_playSink->start();
    qInfo() << "[AdminAudio] Listening to student";
}

void AdminAudioManager::stopListening() {
    if (m_playSink) {
        m_playSink->stop();
        delete m_playSink;
        m_playSink = nullptr;
    }
    m_playDevice = nullptr;
    m_currentListeningSocket = nullptr;
}

void AdminAudioManager::startSpeakingTo(QWebSocket *targetSocket) {
    stopSpeaking();
    m_currentSpeakingSocket = targetSocket;

    const QAudioDevice inputDevice = QMediaDevices::defaultAudioInput();
    if (!inputDevice.isFormatSupported(m_format)) {
        qWarning() << "[AdminAudio] Input format not supported";
        return;
    }
    m_micSource = new QAudioSource(inputDevice, m_format, this);
    m_micDevice = m_micSource->start();
    connect(m_micDevice, &QIODevice::readyRead, this, &AdminAudioManager::onMicrophoneReadyRead);
    qInfo() << "[AdminAudio] Speaking to student";
}

void AdminAudioManager::stopSpeaking() {
    if (m_micSource) {
        m_micSource->stop();
        delete m_micSource;
        m_micSource = nullptr;
    }
    m_micDevice = nullptr;
    m_currentSpeakingSocket = nullptr;
}

void AdminAudioManager::startSpeakingToAll(const QList<QWebSocket *> &sockets) {
    stopSpeaking();
    m_broadcastSockets = sockets;

    const QAudioDevice inputDevice = QMediaDevices::defaultAudioInput();
    if (!inputDevice.isFormatSupported(m_format)) {
        qWarning() << "[AdminAudio] Input format not supported for broadcast";
        return;
    }
    m_micSource = new QAudioSource(inputDevice, m_format, this);
    m_micDevice = m_micSource->start();
    connect(m_micDevice, &QIODevice::readyRead, this, &AdminAudioManager::onMicrophoneReadyRead);
    qInfo() << "[AdminAudio] Speaking to all students";
}

void AdminAudioManager::stopSpeakingToAll() {
    stopSpeaking();
    m_broadcastSockets.clear();
}

void AdminAudioManager::onMicrophoneReadyRead() {
    if (!m_micDevice) return;
    const QByteArray data = m_micDevice->readAll();
    if (data.isEmpty()) return;

    if (m_currentSpeakingSocket) {
        emit audioPlaybackDataReady(m_currentSpeakingSocket, data);
    } else {
        for (QWebSocket *sock : std::as_const(m_broadcastSockets))
            emit audioPlaybackDataReady(sock, data);
    }
}
