#ifndef MAINWINDOWPV_H
#define MAINWINDOWPV_H

#include <QMainWindow>
#include "postazionevoto.h"

namespace Ui {
class MainWindowPV;
}

class MainWindowPV : public QMainWindow
{
    Q_OBJECT
private:

    Ui::MainWindowPV *ui;
    PostazioneVoto * pv;

    enum InterfacciePV{
        attivazione,
        disponibile,
        abilitazione,
        compilazioneSchede,
        votoInviato
    };

public:
    explicit MainWindowPV(QWidget *parent = 0);
    ~MainWindowPV();

private slots:
    void on_exit_button_clicked();

    void on_attiva_button_clicked();


};

#endif // MAINWINDOWPV_H
