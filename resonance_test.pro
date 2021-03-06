QT       += core gui charts serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

win32: RC_ICONS = $$PWD/wave.ico

win32:RC_ICONS += wave.ico

VERSION = 0.4.0.1
QMAKE_TARGET_COMPANY = 'JINR'
QMAKE_TARGET_PRODUCT = 'Resonance test'
QMAKE_TARGET_DESCRIPTION = 'Resonance test for wire tension bench'
# QMAKE_TARGET_COPYRIGHT = JINR (V. Zel)

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    chartview.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    chartview.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
