/*
 * PostazioneVoto.cpp
 *
 *  Created on: 15/gen/2017
 *      Author: giuseppe
 */

#include "postazionevoto.h"
#include <iostream>

using namespace std;

PostazioneVoto::PostazioneVoto() {

    HTAssociato = 0;
    ivCBC = 0;
    symKeyAES = 0;

    //TODO calcolare dall'indirizzo IP
    idPostazioneVoto = 3;


    //connessione all'urna e richiesta di questi dati
    pSchedeVoto = NULL;
    publicKeyRP = 0;

    this->setStatoPV(statiPV::attesa_attivazione);
}

PostazioneVoto::~PostazioneVoto() {
    // TODO Auto-generated destructor stub
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
    this->pv_client = new SSLClient();
    const char * postazioneSeggio = "192.168.56.100"; //ricavare l'IP della postazione seggio a cui la postazione voto appartiene
    cout << "SSL pointer pre-connect: " << this->pv_client->ssl << endl;
    this->pv_client->connectTo(postazioneSeggio);
    cout << "SSL pointer post-connect: " << this->pv_client->ssl << endl;
    this->pv_client->updateStatoPVtoSeggio(pv_client->ssl,postazioneSeggio,this->idPostazioneVoto,this->statoPV);

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

void PostazioneVoto::setHTAssociato(unsigned int tokenCod) {
    this->HTAssociato = tokenCod;
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

int PostazioneVoto::servicesToSeggio() {
    //TODO creare una socket in ascolto per ricevere le richieste dalla postazione seggio
    return 0;
}

//metodi per la cifratura del voto

bool PostazioneVoto::inviaSchedeToUrnaVirtuale() {
    //TODO creazione connessione SSL con l'urna e invio dei dati di voto cifrati e firmati

    return true;
}

void PostazioneVoto::mostraSchede(tinyxml2::XMLDocument *pschedeVoto) {
    //TODO
}

bool PostazioneVoto::enablingPV() {
 return true;
}

