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
    worker.cpp \
    serialport.cpp
OTHER_FILES += README.txt \
    settings.xml \
    www/sensors.php
HEADERS += definitions.h \
    worker.h \
    serialport.h
