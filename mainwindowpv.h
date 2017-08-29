#ifndef MAINWINDOWPV_H
#define MAINWINDOWPV_H

#include <QMainWindow>
#include "postazionevoto.h"
#include "schedavoto.h"
#include "schedacompilata.h"
#include <QObject>
#include <QListWidgetItem>


class PostazioneVoto;
namespace Ui {
class MainWindowPV;
}

class MainWindowPV : public QMainWindow
{
    Q_OBJECT
private:
    vector <SchedaVoto> schedeVotoDaMostrare;
    Ui::MainWindowPV *ui;
    PostazioneVoto * pv;

    unsigned int indiceSchedaDaMostrare;
    vector <SchedaCompilata> schedeCompilate;
    uint numChecked;
    uint numPreferenzeMax;
    bool addingElementToListWidget;

    enum InterfaccePV{
        attivazione,
        disponibile,
        abilitazione,
        compilazioneSchede,
        votoInviato,
        errore_offline
    };
    void mostraScheda();
public:
    explicit MainWindowPV(QWidget *parent = 0);
    ~MainWindowPV();


    void mostraInterfacciaAbilitazioneWithOTP();
    void mostraInterfacciaPostazioneAttiva();

private slots:
    void on_exit_button_clicked();
    void on_attiva_button_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_nextSend_clicked();

    void on_listWidget_scheda_itemChanged(QListWidgetItem *item);

public slots:
    void updateInterfaccia(unsigned int statoPV);
    void messageErrorPassword();
    void receiveSchedeToShow(vector <SchedaVoto> schede);
signals:
    void checkPassKey(QString passKey);
    void stopThreads();
    void needSchede();
    void inviaSchedeCompilate(vector <SchedaCompilata> schede);
};

#endif // MAINWINDOWPV_H
