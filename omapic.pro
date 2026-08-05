QT += core gui qml quick quickcontrols2 dbus
CONFIG += c++17 release
TARGET = omapic
TEMPLATE = app

HEADERS += \
    src/imageprovider.h \
    src/backend.h

SOURCES += \
    src/main.cpp \
    src/imageprovider.cpp \
    src/backend.cpp

RESOURCES += src/resources.qrc
