#include "mainwindowpv.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindowPV w;
    //PostazioneVoto pv;

    w.show();

    return a.exec();
}
