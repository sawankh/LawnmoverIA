#-------------------------------------------------
#
# Project created by QtCreator 2013-10-02T15:22:41
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = IA
TEMPLATE = app

INCLUDEPATH = ./include

SOURCES += src/main.cpp\
        src/mainwindow.cpp \
    src/celda.cpp \
    src/cortadora.cpp

HEADERS  += include/mainwindow.h \
    include/celda.h \
    include/cortadora.h

FORMS    += mainwindow.ui

RESOURCES += \
    Recursos.qrc

#CONFIG += release
