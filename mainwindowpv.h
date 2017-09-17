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
    uint numPreferenzeChecked;
    uint numPreferenzeMax;
    //mettendo a true questa variabile, si blocca il trigger degli
    //eventi checked e unchecked durante l'aggiunta degli elementi
    //alla schermata di visualizzazione delle schede per il voto
    bool addingElementToListWidget;
    enum InterfaccePV{
        attivazione,
        disponibile,
        abilitazione,
        compilazioneSchede,
        votoInviato,
        offline,
        errore
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

    void on_confermaOTP_button_clicked();

public slots:
    void updateInterfaccia(unsigned int statoPV);
    void messageErrorPassword();
    void receiveSchedeToShow(vector <SchedaVoto> schede);
    void showErrorOTP();
    void showMessageUrnaUnreachable();
signals:
    void checkPassKey(QString passKey);
    void stopThreads();
    void needSchede();
    void inviaSchedeCompilate(vector <SchedaCompilata> schede);
    void checkOTP(QString);
};

#endif // MAINWINDOWPV_H
