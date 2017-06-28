#include "mainwindowpv.h"
#include "ui_mainwindowpv.h"
#include <iostream>


using namespace std;

MainWindowPV::MainWindowPV(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindowPV)
{
    ui->setupUi(this);
    pv=new PostazioneVoto(this);
    cout << "View: postazione avviata" << endl;
    ui->wrongPassword_label->hide();
}

MainWindowPV::~MainWindowPV()
{
    delete pv;
    delete ui;
}

void MainWindowPV::on_exit_button_clicked()
{
     QApplication::quit();
}

void MainWindowPV::on_attiva_button_clicked()
{
    ui->wrongPassword_label->hide();
    //TODO
    //usare la password inserita come chiave di sessione per calcolare un valore mac
    //sul valore della procedura di voto e inviarlo all'urna
    QString pass = ui->passwordPV_lineEdit->text();
    if(pass=="pv1"){
        ui->stackedWidget->setCurrentIndex(InterfacciePV::disponibile);
        ui->passwordPV_lineEdit->setText("");
        pv->setStatoPV(PostazioneVoto::libera);
        //avvio server in ascolto del seggio
        pv->runServicesToSeggio();
    }
    else{
        ui->wrongPassword_label->show();
    }
}

void MainWindowPV::mostraInterfacciaAbilitazioneWithOTP(){
    ui->stackedWidget->setCurrentIndex(InterfacciePV::abilitazione);
}

void MainWindowPV::mostraInterfacciaPostazioneAttiva(){
    ui->stackedWidget->setCurrentIndex(InterfacciePV::disponibile);
}
