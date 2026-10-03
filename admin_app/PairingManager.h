#pragma once
#include <QRandomGenerator>
#include <QSettings>
#include <QString>

class PairingManager {
public:

    static constexpr auto ALPHABET = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";
    static constexpr int  CODE_LENGTH = 8;

    static QString savedCode() {
        return QSettings().value(QStringLiteral("pairing/code")).toString();
    }

    static QString generateCode() {
        const QString alphabet = QString::fromLatin1(ALPHABET);
        QString code;
        code.reserve(CODE_LENGTH);
        for (int i = 0; i < CODE_LENGTH; ++i) {
            const int index = QRandomGenerator::system()->bounded(
                static_cast<int>(alphabet.size()));
            code.append(alphabet.at(index));
        }
        QSettings settings;
        settings.setValue(QStringLiteral("pairing/code"), code);

        settings.sync();
        return code;
    }

    static QString codeOrCreate() {
        const QString existing = savedCode();
        return existing.isEmpty() ? generateCode() : existing;
    }

    static bool isValid(const QString &code) {
        const QString expected = savedCode();
        return !expected.isEmpty() && code.trimmed().toUpper() == expected;
    }
};
