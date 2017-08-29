#include "mainwindowpv.h"
#include "ui_mainwindowpv.h"
#include <iostream>
#include <QMessageBox>


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
    QObject::connect(this,SIGNAL(needSchede()),pv,SLOT(selectSchedeDaMostrare()));
    QObject::connect(pv,SIGNAL(giveSchedeToView(vector<SchedaVoto>)),this,SLOT(receiveSchedeToShow(vector<SchedaVoto>)));

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

void MainWindowPV::receiveSchedeToShow(vector <SchedaVoto> schede)
{
    this->schedeVotoDaMostrare = schede;
    //TODO calcola quali e quante schede mostrare all'elettore attivo corrente
    indiceSchedaDaMostrare = 0;
    mostraScheda();

}
void MainWindowPV::mostraScheda(){

    //aggiorniamo testo da mostrare sul bottone per la successiva scheda o l'invio dei voti
    if(indiceSchedaDaMostrare < (schedeVotoDaMostrare.size() - 1)){
        ui->pushButton_nextSend->setText("Prossima Scheda");
    }
    else{
        ui->pushButton_nextSend->setText("Invia all'urna");
    }

    ui->listWidget_scheda->clear();
    QFont serifFont("Times", 20, QFont::Bold);
    SchedaVoto schedaCorrente = schedeVotoDaMostrare.at(indiceSchedaDaMostrare);
    uint codProcedura = schedaCorrente.getIdProceduraVoto();
    //QListWidgetItem * item;
    //item = new QListWidgetItem("id Procedura: " + QString::number(codProcedura),ui->listWidget_scheda);
    //item->setFont(serifFont);
    ui->label_procedura_value->setText(QString::number(codProcedura));

    uint codScheda = schedaCorrente.getId();
    //    item = new QListWidgetItem("Codice scheda: " +
    //                               QString::number(codScheda),ui->listWidget_scheda);
    //    item->setFont(serifFont);
    ui->label_numeroSchedaValue->setText(QString::number(codScheda));

    uint numeroPreferenze = schedaCorrente.getNumPreferenze();
    //    item = new QListWidgetItem("Numero preferenze: " +
    //                               QString::number(numeroPreferenze),ui->listWidget_scheda);
    //    item->setFont(serifFont);
    ui->label_numeroPreferenzeValue->setText(QString::number(numeroPreferenze));
    numPreferenzeMax = numeroPreferenze;
    numChecked = 0;
    cout << "Numero preferenze selezionate: " << numChecked << endl;

    uint tipologiaElezione = schedaCorrente.getTipoElezione();
    ui->label_tipologiaElezioneValue->setText(QString::number(tipologiaElezione));

    vector <ListaElettorale> liste = schedaCorrente.getListeElettorali();
    for (uint listaIndex = 0; listaIndex < liste.size() ; listaIndex++){
        ListaElettorale listaCorrente = liste.at(listaIndex);
        vector <Candidato> candidatiLista = listaCorrente.getCandidati();

        //inserisco nome della lista
        string nomeLista = listaCorrente.getNome();
        if(nomeLista!="nessuna lista"){ //"nessuna lista" è il nome di default dato alla lista, quando il nome della lista non è richiesto per una votazione
            QString infoLista = "Lista " + QString::number(listaIndex+1) + ": " + QString::fromStdString(nomeLista);
            QListWidgetItem * item = new QListWidgetItem(infoLista,ui->listWidget_scheda);
            item->setFont(serifFont);
        }
        for (uint i = 0; i < candidatiLista.size(); i++ ){

            QString nominativo = QString::fromStdString(candidatiLista.at(i).getNome() + " "+ candidatiLista.at(i).getCognome());

            string matricola = candidatiLista.at(i).getMatricola();


            QListWidgetItem* item = new QListWidgetItem(nominativo,ui->listWidget_scheda);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable); // set checkable flag
            item->setCheckState(Qt::Unchecked);
            QVariant matrVariant(QString::fromStdString(matricola));
            item->setData(Qt::UserRole,matrVariant);

            string luogo = candidatiLista.at(i).getLuogoNascita();
            string data = candidatiLista.at(i).getDataNascita();
            QString infoCandidato = QString::fromStdString("nato a " + luogo + " il " + data);
            QVariant infoCandVariant(infoCandidato);
            item->setData(Qt::ToolTipRole,infoCandVariant);
        }


    }

    //visualizzazione scheda completata, azzeriamo il valore numChecked, perché l'aggiunta di elementi nel list widget viene considerata una variazione degli item
    //purtroppo non riesco a catturare l'evento di check sugli item diversamente
    numChecked = 0;
    ui->stackedWidget->setCurrentIndex(InterfaccePV::compilazioneSchede);

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
        //break;
    case pv->statiPV::votazione_in_corso:
        emit needSchede();
        //ui->stackedWidget->setCurrentIndex(InterfaccePV::compilazioneSchede);
        cout << "schede di voto richieste" << endl;
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

void MainWindowPV::on_pushButton_nextSend_clicked()
{
    SchedaCompilata sc;
    //TODO estrai dati dall'interfaccia di compilazione scheda,
    //crea una scheda compilata e aggiungila al vettore delle
    //schede compilate da inviare all'urna

    schedeCompilate.push_back(sc);

    //verifica se bisogna mostra la scheda successiva, o se
    //quella mostrata era l'ultima e bisogna procedere all'invio
    //delle schede compilate
    uint indiceSchedaMostrata = indiceSchedaDaMostrare;
    if(indiceSchedaMostrata < (schedeVotoDaMostrare.size()-1)){
        indiceSchedaDaMostrare++;
        mostraScheda();
    }
    else{
        emit inviaSchedeCompilate(schedeCompilate);
    }
}

void MainWindowPV::on_listWidget_scheda_itemChanged(QListWidgetItem *item)
{
    QVariant var = item->data(Qt::UserRole);
    string matricola = var.toString().toStdString();
    cout << "Item's matricola: " << matricola << endl;
    if(item->checkState()==Qt::CheckState::Checked){
        numChecked++;

    }
    else if (item->checkState()==Qt::CheckState::Unchecked){
        numChecked--;
    }

    if(numChecked>numPreferenzeMax){
        ui->pushButton_nextSend->setEnabled(false);
        QMessageBox msgBox(this);
        msgBox.setInformativeText("Hai superato il numero di preferenze massime consentito. Deseleziona almeno una preferenza.");
        msgBox.exec();

    }
    else{
        ui->pushButton_nextSend->setEnabled(true);
    }
    cout << "Preferenze massime:" << numPreferenzeMax << endl;
    cout << "Numero preferenze selezionate: " << numChecked << endl;
}
