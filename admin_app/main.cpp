#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include "DiscoveryBroadcaster.h"
#include "PairingManager.h"
#include "adminserver.h"
#include "networkutils.h"

using namespace Qt::StringLiterals;

namespace {
constexpr quint16 WEBSOCKET_PORT = 9000;
}

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("Classroom"));
    app.setApplicationName(QStringLiteral("Teacher"));

    const bool firstRun = PairingManager::savedCode().isEmpty();
    const QString code  = PairingManager::codeOrCreate();
    if (firstRun)
        qInfo() << "[Admin] First run, generated a new pairing code.";
    qInfo() << "[Admin] Pairing code:" << code << "at" << localIPv4();

    AdminServer server(WEBSOCKET_PORT);
    DiscoveryBroadcaster broadcaster(WEBSOCKET_PORT);
    if (server.isListening())
        broadcaster.start();

    QObject::connect(&server, &AdminServer::clientAuthenticated,
                     [](const QString &host, const QString &user)
                     {
                         qInfo() << "[Admin] Student joined:" << host << user;
                     });
    QObject::connect(&server, &AdminServer::clientDisconnected,
                     [](const QString &host)
                     {
                         qInfo() << "[Admin] Student left:" << host;
                     });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("adminServer"), &server);
    engine.rootContext()->setContextProperty(QStringLiteral("streamManager"), server.streamManager());

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.load(QUrl(u"qrc:/Teacher/main.qml"_s));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
