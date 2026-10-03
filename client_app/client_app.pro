QT += quick quickcontrols2 quickeffects network websockets multimedia

TARGET = Student

CONFIG += c++17
CONFIG -= app_bundle

SOURCES += \
    ClientAudioManager.cpp \
    ClientBridge.cpp \
    WebSocketClient.cpp \
    main.cpp

HEADERS += \
    ClientAudioManager.h \
    ClientBridge.h \
    DiscoveryListener.h \
    HeartbeatService.h \
    ScreenCapturer.h \
    WebSocketClient.h

RESOURCES += client.qrc

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

include(../shared/shared.pri)
