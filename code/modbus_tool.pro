
QT       += core gui serialport charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DESTDIR += ../bin
#OBJECTS_DIR = ../bin/obj
TARGET = modbus_tool
TEMPLATE = app
DEFINES -= UNICODE

CONFIG += c++11
CONFIG += warn_off  #不要啥都提示

#QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
#QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += ./src .
DEFINES += NO_MAIN

msvc:QMAKE_CXXFLAGS += /utf-8
msvc:QMAKE_CFLAGS += /utf-8
win32 {
	LIBS += -lwsock32 
	DEFINES += WIN32
}

SOURCES += \
        mainwindow.cpp \
        appflow.cpp \
    ./src/comm_pack.cpp \
    ./src/common.cpp \
    ./src/jsoncpp.cpp \
    ./src/modbus.cpp \
    ./main.cpp \ 
    src/hexstr.cpp

HEADERS += \
        mainwindow.h \
        appflow.h \
    ./src/comm_pack.h \
    ./src/common.h \
    ./src/json.h \
    ./src/modbus.h \
    ./src/main.h \
    mqchart.h \
    colorlog.h \
    src/hexstr.h \
    src/queue_cpp.h

FORMS += \
        mainwindow.ui 

RESOURCES += \
    res.qrc

DISTFILES +=

