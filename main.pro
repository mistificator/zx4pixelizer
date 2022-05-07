QT       += core gui widgets

CONFIG += c++11

TARGET = zx4pixelizer
DESTDIR = ./bin

SOURCES += \
    cie/CIEDE2000.cpp \
    cie/color_conversions.cpp \
    lz4/lz4.c \
    lz4/lz4hc.c \
    main.cpp \
    mainwindow_events.cpp \
    mainwindow_private.cpp \
    zx4pixelizer.cpp \
    mainwindow.cpp \
    scenewindow.cpp \
    spritewindow.cpp

HEADERS += \
    cie/CIEDE2000.h \
    cie/color_conversions.h \
    lz4/lz4.h \
    lz4/lz4hc.h \
    zx4pixelizer.h \
    mainwindow.h \
    scenewindow.h \
    spritewindow.h

FORMS += \
    mainwindow.ui \
    scenewindow.ui \
    spritewindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc

win32:RC_ICONS += resources/sonic.zx4.ico
VERSION = 0.0.0.0
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

QMAKE_CXXFLAGS+= -openmp

INCLUDEPATH += cie lz4
