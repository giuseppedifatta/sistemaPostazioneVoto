/*
 * PostazioneVoto.cpp
 *
 *  Created on: 15/gen/2017
 *      Author: giuseppe
 */

#include "postazionevoto.h"
#include <iostream>

using namespace std;

PostazioneVoto::PostazioneVoto(MainWindowPV *m) {
    mainWindow = m;
    HTAssociato = 0;
    ivCBC = 0;
    symKeyAES = 0;

    //TODO calcolare dall'indirizzo IP
    idPostazioneVoto = 2;


    //connessione all'urna e richiesta di questi dati
    pSchedeVoto = NULL;
    publicKeyRP = 0;
    //init client
    this->pv_client = new SSLClient();


    this->setStatoPV(statiPV::attesa_attivazione);

    this->runServerPV = true;
}

PostazioneVoto::~PostazioneVoto() {
    // TODO Auto-generated destructor stub
    server_thread.join();
    delete this->pv_client;
}

bool PostazioneVoto::PostazioneVoto::offlinePV() {
    // se è possibile comunicare con l'Urna Virtuale ritorna true;
    //se urna non raggiungibile
    this->setStatoPV(statiPV::offline);
    return true;
}

void PostazioneVoto::setStatoPV(statiPV nuovoStato) {
    this->statoPV = nuovoStato;

    //---bisogna comunicare alla postazione seggio che lo stato della postazione di voto X è cambiato---
    //iniziare una sessione ssl con la postazione di voto


    const char * postazioneSeggio = "192.168.192.130"; //ricavare l'IP della postazione seggio a cui la postazione voto appartiene1
    cout << "PV: SSL pointer pre-connect: " << this->pv_client->ssl << endl;
    this->pv_client->connectTo(postazioneSeggio);
    cout << "PV: SSL pointer post-connect: " << this->pv_client->ssl << endl;
    this->pv_client->updateStatoPVtoSeggio(postazioneSeggio,this->idPostazioneVoto,this->statoPV);


}

unsigned int PostazioneVoto::getStatoPV(){
    return (int) this->statoPV;
}

string PostazioneVoto::getStatoPostazioneAsString() {
    string stato;
    switch (this->statoPV) {
    case (attesa_attivazione):
        stato = "in attesa di attivazione con password";
        break;
    case (libera):
        stato = "postazione libera";
        break;
    case (attesa_abilitazione):
        stato = "in attesa di abilitazione con OTP";
        break;
    case (votazione_in_corso):
        stato = "in corso";
        break;
    case (votazione_completata):
        stato = "votazione completata";
        break;
    case (errore):
        stato = "errore";
        break;
    case (offline):
        stato = "offline";
        break;

    }
    return stato;
}

bool PostazioneVoto::voteAuthorizationWithOTP() {
    //richiamare interfaccia di verifica dell'OTP
    return true;
}

int PostazioneVoto::setHTAssociato(unsigned int tokenCod) {
    if(this->HTAssociato == 0){
        this->HTAssociato = tokenCod;

        //TODO contattare l'otp Server Provider per comunicare l'id dell'HT da abbinare ad una certa postazione di voto
        //mainWindow->mostraInterfacciaAbilitazioneWithOTP();
        cout << "PV: aggiorno lo stato della postazione di voto..." << endl;
        this->setStatoPV(this->statiPV::attesa_abilitazione);
        cout << "PV: stato postazione di voto aggiornato." << endl;
        return 0;
    }
    else{
        cout << "PV: Resetto l'ht della postazione" << endl;
        this->HTAssociato = tokenCod;
        return 1;
    }
}

unsigned int PostazioneVoto::getHTAssociato() {
    return this->HTAssociato;
}

unsigned int PostazioneVoto::getIdPostazioneVoto() {
    return this->idPostazioneVoto;
}

unsigned int PostazioneVoto::getPublicKeyPV() {
    return this->publicKeyPV;
} //servizio per l'urna, inutile se le SmartCard delle postazioni voto sono prememorizzate nell'urna, se la PV invia l'id relativo alla SC che è inserita

void PostazioneVoto::compilaScheda() {
    //TODO
} //estrae i dati dalla schermata di compilazione di una singola scheda e inserisce un elemento nel vettore delle schedeCompilate

int PostazioneVoto::runServicesToSeggio() {

    server_thread = thread(&PostazioneVoto::runServerListenSeggio, this);
    return 0;
}

//metodi per la cifratura del voto

bool PostazioneVoto::inviaSchedeToUrnaVirtuale() {
    //TODO creazione connessione SSL con l'urna e invio dei dati di voto cifrati e firmati

    return true;
}

//void PostazioneVoto::mostraSchede(tinyxml2::XMLDocument *pschedeVoto) {
//    //TODO
//}

bool PostazioneVoto::enablingPV() {
    return true;
}

void PostazioneVoto::runServerListenSeggio(){
    pv_server = new SSLServer(this);
    this->mutex_stdout.lock();
    cout << "PV: avvio del pv_server per rispondere alle richieste del seggio" << endl;
    this->mutex_stdout.unlock();
    while(this->runServerPV){
        //attesa di una richiesta dal seggio
        this->pv_server->ascoltaSeggio();
        //prosegue rimettendosi in ascolto al ciclo successivo, se runServerPV ha valore booleano true
    }

    this->mutex_stdout.lock();
    cout << "PV: runServerListenSeggio: exit!" << endl;
    this->mutex_stdout.unlock();

    delete this->pv_server;
    // il thread che eseguiva la funzione termina se la funzione arriva alla fine
    return;

}

void PostazioneVoto::backToPostazioneAttiva(){
    //mainWindow->mostraInterfacciaPostazioneAttiva();
}

void PostazioneVoto::stopServerPV(){
    this->runServerPV = false;
    //predispongo il server per l'interruzione

    this->pv_server->setStopServer(true);

    this->mutex_stdout.lock();
    cout << "PV: il ServerPV sta per essere fermato" << endl;
    this->mutex_stdout.unlock();

    //mi connetto al server locale per sbloccare l'ascolto e portare alla terminazione della funzione eseguita dal thread che funge da serve in ascolto
    const char * localhost = "127.0.0.1";

    //implementare----->>>>
    this->pv_client->stopLocalServer(localhost);
}
