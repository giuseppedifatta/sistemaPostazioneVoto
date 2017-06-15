#include "mainwindowpv.h"
#include "ui_mainwindowpv.h"
#include <iostream>

using namespace std;

MainWindowPV::MainWindowPV(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindowPV)
{
    ui->setupUi(this);
    pv=new PostazioneVoto();
    cout << "postazione avviata" << endl;
    ui->wrongPassword_label->hide();
}

MainWindowPV::~MainWindowPV()
{
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

    }
    else{
        ui->wrongPassword_label->show();
    }
}

