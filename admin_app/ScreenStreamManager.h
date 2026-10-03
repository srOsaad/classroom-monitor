#pragma once
#include <QByteArray>
#include <QMap>
#include <QObject>
#include <QVariant>
#include <QVariantList>
#include <QWebSocket>
#include "ClientRegistry.h"

class StudentStream : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString frame       READ frame       NOTIFY frameChanged)
    Q_PROPERTY(QString cameraFrame READ cameraFrame NOTIFY cameraFrameChanged)
    Q_PROPERTY(bool    connected   READ connected   NOTIFY connectedChanged)
    Q_PROPERTY(QString hostname    READ hostname    CONSTANT)
    Q_PROPERTY(QString username    READ username    CONSTANT)
public:
    StudentStream(const QString &hostname, const QString &username, QObject *parent = nullptr)
        : QObject(parent), m_hostname(hostname), m_username(username) {}

    QString frame()       const { return m_frame; }
    QString cameraFrame() const { return m_cameraFrame; }
    bool    connected()   const { return m_connected; }
    QString hostname()    const { return m_hostname; }
    QString username()    const { return m_username; }

    void updateFrame(const QByteArray &jpeg) {

        m_frame = QStringLiteral("data:image/jpeg;base64,") +
                  QString::fromLatin1(jpeg.toBase64());
        emit frameChanged();
    }

    void updateCameraFrame(const QByteArray &jpeg) {
        m_cameraFrame = QStringLiteral("data:image/jpeg;base64,") +
                        QString::fromLatin1(jpeg.toBase64());
        emit cameraFrameChanged();
    }

    void setConnected(bool c) {
        if (m_connected == c) return;
        m_connected = c;
        if (!c) {
            m_frame.clear();
            m_cameraFrame.clear();
            emit frameChanged();
            emit cameraFrameChanged();
        }
        emit connectedChanged();
    }

signals:
    void frameChanged();
    void cameraFrameChanged();
    void connectedChanged();

private:
    QString m_hostname;
    QString m_username;
    QString m_frame;
    QString m_cameraFrame;
    bool    m_connected = true;
};

class ScreenStreamManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList students READ students NOTIFY studentsChanged)
public:
    explicit ScreenStreamManager(ClientRegistry *registry, QObject *parent = nullptr)
        : QObject(parent), m_registry(registry) {}

    void addStudent(QWebSocket *socket) {
        ClientInfo &info = m_registry->get(socket);
        const QString key = info.hostname;

        if (m_byHostname.contains(key)) {
            StudentStream *stream = m_byHostname[key];
            m_bySocket[socket] = stream;
            stream->setConnected(true);
        } else {
            auto *stream = new StudentStream(info.hostname, info.username, this);
            m_byHostname[key]  = stream;
            m_bySocket[socket] = stream;
            emit studentsChanged();
        }
    }

    void onFrameReceived(QWebSocket *socket, const QByteArray &jpeg) {
        if (m_bySocket.contains(socket))
            m_bySocket[socket]->updateFrame(jpeg);
    }

    void onCameraFrameReceived(QWebSocket *socket, const QByteArray &jpeg) {
        if (m_bySocket.contains(socket))
            m_bySocket[socket]->updateCameraFrame(jpeg);
    }

    void removeStudent(QWebSocket *socket) {
        if (!m_bySocket.contains(socket)) return;
        m_bySocket[socket]->setConnected(false);
        m_bySocket.remove(socket);
    }

    QVariantList students() const {
        QVariantList list;
        for (StudentStream *s : m_byHostname.values())
            list.append(QVariant::fromValue(s));
        return list;
    }

signals:
    void studentsChanged();

private:
    ClientRegistry *m_registry;
    QMap<QString, StudentStream *>     m_byHostname;
    QMap<QWebSocket *, StudentStream *> m_bySocket;
};
