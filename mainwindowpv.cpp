#include "mainwindowpv.h"
#include "ui_mainwindowpv.h"
#include <iostream>


using namespace std;

MainWindowPV::MainWindowPV(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindowPV)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(InterfaccePV::attivazione);
    ui->wrongPassword_label->hide();

    //inizializzo il model
    pv = new PostazioneVoto(this);

    QObject::connect(pv, SIGNAL (stateChange(unsigned int)), this, SLOT (updateInterfaccia(unsigned int)));
    QObject::connect(this, SIGNAL(checkPassKey(QString)),pv, SLOT(validatePassKey(QString)));
    QObject::connect(this, SIGNAL(stopThreads()), pv, SLOT(stopServerPV()));
    QObject::connect(pv, SIGNAL(wrongPassKey()),this, SLOT(messageErrorPassword()));

    //avvio il thread del model
    pv->start();

    cout << "View: postazione avviata" << endl;

}

MainWindowPV::~MainWindowPV()
{
    pv->wait();
    delete ui;
}

void MainWindowPV::on_exit_button_clicked()
{
    emit stopThreads();
    QApplication::quit();
}

void MainWindowPV::on_attiva_button_clicked()
{
    ui->wrongPassword_label->hide();
    //TODO
    //usare la password inserita come chiave di sessione per calcolare un valore mac
    //sul valore della procedura di voto e inviarlo all'urna
    QString passKey = ui->passwordPV_lineEdit->text();

    //segnalazione al model, per il controllo della passKey
    emit checkPassKey(passKey);

    //spostare su una funzione di tipo public slots, chiamata dalla postazione attraverso un signale, passando il valore bool riguardante l'esito della verifica della passKey
    //    if(passKey=="pv1"){
    //        ui->stackedWidget->setCurrentIndex(InterfaccePV::disponibile);
    //
    //        pv->setStatoPV(PostazioneVoto::libera);
    //        cout << "View: postazione attiva" << endl;
    //        //avvio server in ascolto del seggio
    //        pv->runServicesToSeggio();
    //    }
    //    else{
    //
    //    }
}

void MainWindowPV::mostraInterfacciaAbilitazioneWithOTP(){
    ui->stackedWidget->setCurrentIndex(InterfaccePV::abilitazione);
}

void MainWindowPV::mostraInterfacciaPostazioneAttiva(){
    ui->stackedWidget->setCurrentIndex(InterfaccePV::disponibile);
}

void MainWindowPV::on_pushButton_2_clicked()
{
    //chiusura app temporanea


    //pv->stopServerPV();
    emit stopThreads();

    QApplication::quit();
}

void MainWindowPV::updateInterfaccia(unsigned int statoPV){
    //int statoPV = pv->getStatoPV();

    switch(statoPV){
    case pv->statiPV::attesa_attivazione:
        ui->stackedWidget->setCurrentIndex(InterfaccePV::attivazione);
        cout << "schermata attivazione impostata" << endl;
        break;
    case pv->statiPV::libera:
        ui->stackedWidget->setCurrentIndex(InterfaccePV::disponibile);
        ui->passwordPV_lineEdit->setText("");
        ui->wrongPassword_label->hide();
        cout << "schermata disponibile impostata" << endl;
        break;
    case pv->statiPV::attesa_abilitazione:
        ui->stackedWidget->setCurrentIndex(InterfaccePV::abilitazione);
        cout << "schermata abilitazione otp impostata" << endl;
        break;
    case pv->statiPV::votazione_in_corso:
        ui->stackedWidget->setCurrentIndex(InterfaccePV::compilazioneSchede);
        break;
    case pv->statiPV::votazione_completata:
        ui->stackedWidget->setCurrentIndex(InterfaccePV::votoInviato);
        break;
    case pv->statiPV::errore:
    case pv->statiPV::offline:
        ui->stackedWidget->setCurrentIndex(InterfaccePV::errore_offline);
        break;
    }
    cout << "View: interfaccia aggiornata" << endl;
}

void MainWindowPV::messageErrorPassword(){
    ui->wrongPassword_label->show();
}
