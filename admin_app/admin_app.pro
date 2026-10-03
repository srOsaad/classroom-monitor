QT += quick quickcontrols2 quickeffects network websockets multimedia

TARGET = Teacher

CONFIG += c++17 console
CONFIG -= app_bundle

SOURCES += \
    AdminAudioManager.cpp \
    adminserver.cpp \
    main.cpp

HEADERS += \
    AdminAudioManager.h \
    ClientRegistry.h \
    DiscoveryBroadcaster.h \
    PairingManager.h \
    ScreenStreamManager.h \
    adminserver.h

RESOURCES += admin.qrc

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

include(../shared/shared.pri)
