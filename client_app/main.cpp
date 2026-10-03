#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include "ClientBridge.h"

using namespace Qt::StringLiterals;

namespace {

constexpr auto AUTO_START_KEY = "ClassroomStudent";

#ifdef Q_OS_WIN
bool autoStartMatchesThisExe() {
    QSettings run(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
        QSettings::NativeFormat);
    if (!run.contains(QLatin1String(AUTO_START_KEY)))
        return false;
    const QString saved = run.value(QLatin1String(AUTO_START_KEY)).toString();
    const QString current =
        QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    return saved.compare(current, Qt::CaseInsensitive) == 0;
}

void enableAutoStart() {
    if (autoStartMatchesThisExe())
        return;
    QSettings run(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
        QSettings::NativeFormat);
    run.setValue(QLatin1String(AUTO_START_KEY),
                 QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
}
#endif

}

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("Classroom"));
    app.setApplicationName(QStringLiteral("Student"));

#ifdef Q_OS_WIN
    enableAutoStart();
#endif

    if (argc >= 2) {
        const QString code = QString::fromUtf8(argv[1]).trimmed().toUpper();
        QSettings s;
        s.setValue(QStringLiteral("admin/code"), code);
        s.setValue(QStringLiteral("admin/code_hash"), hashCode(code));
        qInfo() << "[Client] Pairing code set from command line.";
    }

    ClientBridge bridge;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("clientBridge"), &bridge);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.load(QUrl(u"qrc:/Student/main.qml"_s));
    if (engine.rootObjects().isEmpty())
        return -1;

    bridge.connectIfPaired();

    return app.exec();
}
