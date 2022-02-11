QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    zx4pixelizer.cpp \
    mainwindow.cpp \
    scenewindow.cpp \
    spritewindow.cpp

HEADERS += \
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

win32:RC_ICONS += sonic.zx4.ico
VERSION = 1.0.0.0
DEFINES += APP_VERSION=\\\"$$VERSION\\\"
