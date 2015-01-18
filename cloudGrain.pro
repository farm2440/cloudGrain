# -------------------------------------------------
# Project created by QtCreator 2014-12-12T23:21:46
# -------------------------------------------------
QT += network \
    xml
QT -= gui
TARGET = cloudGrain
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += main.cpp \
    worker.cpp
OTHER_FILES += README.txt \
    settings.xml \
    www/sensors.php \
    README.md \
    script/setEnv \
    script/snum.txt
HEADERS += definitions.h \
    worker.h

# За QSerialDevice
DEPENDPATH += .
INCLUDEPATH += ./qserialdevice/src
QMAKE_LIBDIR += ./qserialdevice/src/release
LIBS += -lqserialdevice
