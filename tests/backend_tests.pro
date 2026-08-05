QT += core gui quick testlib
CONFIG += c++17 testcase
TARGET = backend_tests
TEMPLATE = app

INCLUDEPATH += ../src

HEADERS += ../src/backend.h ../src/filepicker.h
SOURCES += backend_tests.cpp ../src/backend.cpp ../src/imageprovider.cpp
