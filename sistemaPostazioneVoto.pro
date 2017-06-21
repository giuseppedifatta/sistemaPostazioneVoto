#-------------------------------------------------
#
# Project created by QtCreator 2017-06-07T10:23:30
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sistemaPostazioneVoto
TEMPLATE = app


SOURCES += main.cpp\
        mainwindowpv.cpp \
    postazionevoto.cpp \
    sslclient.cpp \
    sslserver.cpp

HEADERS  += mainwindowpv.h \
    postazionevoto.h \
    sslclient.h \
    sslserver.h

FORMS    += mainwindowpv.ui

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../usr/lib/x86_64-linux-gnu/release/ -lssl
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../usr/lib/x86_64-linux-gnu/debug/ -lssl
else:unix: LIBS += -L$$PWD/../../../../usr/lib/x86_64-linux-gnu/ -lssl


INCLUDEPATH += $$PWD/../../../../usr/lib/x86_64-linux-gnu
DEPENDPATH += $$PWD/../../../../usr/lib/x86_64-linux-gnu

LIBS += \
    -lssl \
    -lcrypto

