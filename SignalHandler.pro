#-------------------------------------------------
#
# Project created by QtCreator 2019-07-11T19:02:10
#
#-------------------------------------------------

QT       += core gui
CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SignalHandler
TEMPLATE = app


SOURCES += main.cpp\
    signalhandler.cpp \
    widget.cpp

HEADERS  += \
    signalhandler.h \
    widget.h

FORMS    += \
    widget.ui
