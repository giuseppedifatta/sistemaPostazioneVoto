#ifndef MAINWINDOWPV_H
#define MAINWINDOWPV_H

#include <QMainWindow>
#include "postazionevoto.h"
#include <QObject>

class PostazioneVoto;
namespace Ui {
class MainWindowPV;
}

class MainWindowPV : public QMainWindow
{
    Q_OBJECT
private:

    Ui::MainWindowPV *ui;
    PostazioneVoto * pv;

    enum InterfaccePV{
        attivazione,
        disponibile,
        abilitazione,
        compilazioneSchede,
        votoInviato,
        errore_offline
    };

public:
    explicit MainWindowPV(QWidget *parent = 0);
    ~MainWindowPV();


    void mostraInterfacciaAbilitazioneWithOTP();
    void mostraInterfacciaPostazioneAttiva();
private slots:
    void on_exit_button_clicked();
    void on_attiva_button_clicked();
    void on_pushButton_2_clicked();

public slots:
    void updateInterfaccia(unsigned int statoPV);
    void messageErrorPassword();
signals:
    void checkPassKey(QString passKey);
    void stopThreads();
};

#endif // MAINWINDOWPV_H
