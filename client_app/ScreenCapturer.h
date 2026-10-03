#pragma once
#include <QBuffer>
#include <QDebug>
#include <QGuiApplication>
#include <QObject>
#include <QPixmap>
#include <QScreen>
#include <QTimer>
#include "WebSocketClient.h"
#include "protocol.h"

class ScreenCapturer : public QObject {
    Q_OBJECT
public:
    explicit ScreenCapturer(WebSocketClient *client, QObject *parent = nullptr)
        : QObject(parent), m_client(client)
    {
        m_timer = new QTimer(this);
        m_timer->setInterval(143);
        connect(m_timer, &QTimer::timeout, this, &ScreenCapturer::captureAndSend);
    }

    void start() {
        if (!m_timer->isActive()) {
            qInfo() << "[ScreenCapturer] Starting capture at ~7 FPS";
            m_timer->start();
        }
    }

    void stop() {
        if (m_timer->isActive()) {
            qInfo() << "[ScreenCapturer] Stopping capture";
            m_timer->stop();
        }
    }

    void setQuality(int maxWidth, int jpegQuality) {
        m_maxWidth    = maxWidth;
        m_jpegQuality = jpegQuality;
        qInfo() << "[ScreenCapturer] Quality maxWidth=" << maxWidth
                << "quality=" << jpegQuality;
    }

    void setCaptureInterval(int intervalMs) {
        const bool wasRunning = m_timer->isActive();
        if (wasRunning) stop();
        m_timer->setInterval(intervalMs);
        if (wasRunning) start();
    }

    bool isRunning() const { return m_timer->isActive(); }

private slots:
    void captureAndSend() {
        if (!m_client->isAuthenticated()) return;

        QScreen *screen = QGuiApplication::primaryScreen();
        if (!screen) return;

        QPixmap pixmap = screen->grabWindow(0);
        if (pixmap.isNull()) return;

        if (pixmap.width() > m_maxWidth)
            pixmap = pixmap.scaledToWidth(m_maxWidth, Qt::SmoothTransformation);

        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        pixmap.save(&buffer, "JPEG", m_jpegQuality);
        buffer.close();
        if (bytes.isEmpty()) return;

        QByteArray message;
        message.append(BIN_SCREEN_FRAME);
        message.append(bytes);
        m_client->sendBinary(message);
    }

private:
    WebSocketClient *m_client;
    QTimer          *m_timer;
    int m_maxWidth    = 1280;
    int m_jpegQuality = 40;
};
