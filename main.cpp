#include "mainwindowpv.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindowPV w;
    w.show();

    return a.exec();
}
