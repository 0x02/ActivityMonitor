#-------------------------------------------------
#
# Project created by QtCreator 2013-06-30T16:12:07
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ActivityMonitor
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp \
    PieWidget.cpp \
    FBSDMemory.cpp

HEADERS  += MainWindow.h \
    PieWidget.h \
    IActivity.h \
    FBSDMemory.h \
    FreeBSD.h \
    FormatSize.h

FORMS    += MainWindow.ui \
    FBSDMemory.ui

LIBS += -lkvm -lc++ -lcxxrt
QMAKE_CXXFLAGS += -std=c++11 -stdlib=libc++
QMAKE_LFLAGS = -std=c++11 -stdlib=libc++
