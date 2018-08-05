
QT       += core gui serialport charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DESTDIR += ./bin
OBJECTS_DIR = ./bin/obj
TARGET = modbusdis
TEMPLATE = app
DEFINES -= UNICODE

CONFIG += c++11
CONFIG += warn_off  #不要啥都提示

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += ./src .
DEFINES += NO_MAIN

win32 {
	LIBS += -lwsock32 
	DEFINES += WIN32
}

SOURCES += \
        mainwindow.cpp \
    ./src/comm_pack.cpp \
    ./src/common.cpp \
    ./src/jsoncpp.cpp \
    ./src/modbus.cpp \
    ./main.cpp 

HEADERS += \
        mainwindow.h \
    ./src/comm_pack.h \
    ./src/common.h \
    ./src/json.h \
    ./src/modbus.h \
    ./src/main.h 

FORMS += \
        mainwindow.ui 

