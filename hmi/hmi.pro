QT += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET   = hmi
TEMPLATE = app
CONFIG  += c++11

SOURCES += \
    main.cpp \
    commworker.cpp \
    mainwindow.cpp

HEADERS += \
    commworker.h \
    mainwindow.h
