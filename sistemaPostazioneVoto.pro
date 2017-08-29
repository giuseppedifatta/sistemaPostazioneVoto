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
    sslserver.cpp \
    schedavoto.cpp \
    candidato.cpp \
    listaelettorale.cpp \
    schedacompilata.cpp

HEADERS  += mainwindowpv.h \
    postazionevoto.h \
    sslclient.h \
    sslserver.h \
    schedavoto.h \
    candidato.h \
    listaelettorale.h \
    schedacompilata.h

FORMS    += mainwindowpv.ui

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../usr/lib/x86_64-linux-gnu/release/ -lssl
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../usr/lib/x86_64-linux-gnu/debug/ -lssl
else:unix: LIBS += -L$$PWD/../../../../usr/lib/x86_64-linux-gnu/ -lssl


INCLUDEPATH += $$PWD/../../../../usr/lib/x86_64-linux-gnu
DEPENDPATH += $$PWD/../../../../usr/lib/x86_64-linux-gnu

LIBS += \
    -lssl \
    -lcrypto\
    -lcryptopp


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../usr/lib/x86_64-linux-gnu/release/ -lcryptopp
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../usr/lib/x86_64-linux-gnu/debug/ -lcryptopp
else:unix: LIBS += -L$$PWD/../../../../usr/lib/x86_64-linux-gnu/ -lcryptopp

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../usr/lib/i386-linux-gnu/release/ -ltinyxml2
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../usr/lib/i386-linux-gnu/debug/ -ltinyxml2
else:unix: LIBS += -L$$PWD/../../../../usr/lib/i386-linux-gnu/ -ltinyxml2

