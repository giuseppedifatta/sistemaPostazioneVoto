/*
 * PostazioneVoto.cpp
 *
 *  Created on: 15/gen/2017
 *      Author: giuseppe
 */

#include "postazionevoto.h"
#include <iostream>
#include "cryptopp/osrng.h"
#include "cryptopp/cryptlib.h"
#include "cryptopp/hmac.h"
#include "cryptopp/sha.h"
#include "cryptopp/hex.h"
#include "cryptopp/filters.h"
#include "cryptopp/secblock.h"


using namespace CryptoPP;

using namespace std;



PostazioneVoto::PostazioneVoto(QObject *parent) :
    QThread(parent){
    //mainWindow = m;
    HTAssociato = 0;
    ivCBC = 0;
    symKeyAES = 0;

    //TODO calcolare dall'indirizzo IP
    idPostazioneVoto = 1;


    //connessione all'urna e richiesta di questi dati

    publicKeyRP = 0;

    //init client
    //this->pv_client = new SSLClient(this);

    //abilito l'avvio del server in ascolto
    mutex_run_server.lock();
    this->runServerPV = true;
    mutex_run_server.unlock();
}

PostazioneVoto::~PostazioneVoto() {
    // TODO Auto-generated destructor stub
    // delete this->pv_client;
}

bool PostazioneVoto::PostazioneVoto::offlinePV() {
    // se è possibile comunicare con l'Urna Virtuale ritorna true;
    //se urna non raggiungibile
    this->setStatoPV(statiPV::offline);
    return true;
}

void PostazioneVoto::setStatoPV(statiPV nuovoStato) {

    //dovrei usare un mutex
    this->statoPV = nuovoStato;

    //emetto il segnale che comunica il cambiamento di stato della postazione di voto
    emit stateChange(nuovoStato);
    this->mutex_stdout.lock();
    cout << "PV: segnalato alla view che lo stato della postazione è cambiato"  << endl;
    this->mutex_stdout.unlock();

    //avvio il thread che si occupa di comunicare il nuovo stato al Seggio
    std::thread t(&PostazioneVoto::function_thread_sendStatoToSeggio, this,nuovoStato);
    t.detach();
}

unsigned int PostazioneVoto::getStatoPV(){
    return (int) this->statoPV;
}

//string PostazioneVoto::getStatoPostazioneAsString() {
//    string stato;
//    switch (this->statoPV) {
//    case (attesa_attivazione):
//        stato = "in attesa di attivazione con password";
//        break;
//    case (libera):
//        stato = "postazione libera";
//        break;
//    case (attesa_abilitazione):
//        stato = "in attesa di abilitazione con OTP";
//        break;
//    case (votazione_in_corso):
//        stato = "in corso";
//        break;
//    case (votazione_completata):
//        stato = "votazione completata";
//        break;
//    case (errore):
//        stato = "errore";
//        break;
//    case (offline):
//        stato = "offline";
//        break;

//    }
//    return stato;
//}

bool PostazioneVoto::voteAuthorizationWithOTP() {
    //richiamare interfaccia di verifica dell'OTP
    return true;
}

bool PostazioneVoto::setHTAssociato(unsigned int tokenCod) {
    if(this->HTAssociato == 0){ //nessun token associato
        this->HTAssociato = tokenCod;

        //TODO contattare l'otp Server Provider per comunicare l'id dell'HT da abbinare ad una certa postazione di voto

        return true;
    }
    else{
        cerr << "PV: errore di impostazione dell'HT" << endl;
        return false;
    }
}

void PostazioneVoto::resetHT()
{
    this->HTAssociato = 0;
}

unsigned int PostazioneVoto::getHTAssociato() {
    return this->HTAssociato;
}

unsigned int PostazioneVoto::getIdPostazioneVoto() {
    return this->idPostazioneVoto;
}


void PostazioneVoto::compilaScheda() {
    //TODO
} //estrae i dati dalla schermata di compilazione di una singola scheda e inserisce un elemento nel vettore delle schedeCompilate

void PostazioneVoto::runServicesToSeggio() {

    server_thread = std::thread(&PostazioneVoto::runServerListenSeggio, this);

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

    mutex_run_server.lock();
    bool running = this->runServerPV;
    mutex_run_server.unlock();

    while(running){
        //attesa di una richiesta dal seggio
        this->pv_server->ascoltaSeggio();
        //prosegue rimettendosi in ascolto al ciclo successivo, se runServerPV ha valore booleano true

        mutex_run_server.lock();
        running = this->runServerPV;
        mutex_run_server.unlock();
    }

    this->mutex_stdout.lock();
    cout << "PV: runServerListenSeggio: exit!" << endl;
    this->mutex_stdout.unlock();

    delete this->pv_server;


}

void PostazioneVoto::backToPostazioneAttiva(){
    //mainWindow->mostraInterfacciaPostazioneAttiva();
}

void PostazioneVoto::stopServerPV(){
    mutex_run_server.lock();
    this->runServerPV = false;
    mutex_run_server.unlock();
    //predispongo il server per l'interruzione

    this->pv_server->setStopServer(true);

    this->mutex_stdout.lock();
    cout << "PV: il ServerPV sta per essere fermato" << endl;
    this->mutex_stdout.unlock();

    //mi connetto al server locale per sbloccare l'ascolto e portare alla terminazione della funzione eseguita dal thread che funge da serve in ascolto
    SSLClient * pv_client = new SSLClient(this);
    pv_client->stopLocalServer();
    delete pv_client;

}

void PostazioneVoto::validatePassKey(QString pass)
{
    //contatto l'urna per validare la passward
    const char * ipUrna = "192.168.56.100"; //ricavare l'IP della postazione seggio a cui la postazione voto appartiene1

    SSLClient * pv_client = new SSLClient(this);

    if(pv_client->connectTo(ipUrna)!=nullptr){

        if(pv_client->attivaPostazioneVoto(pass.toStdString())){
            this->setStatoPV(statiPV::libera);
            sessionKey_PV_Urna = pass.toStdString();
        }
        else{
            emit wrongPassKey();
        }
    }



}


void PostazioneVoto::run(){
    this->setStatoPV(statiPV::attesa_attivazione);

    //avvia il server thread di ascolto per fornire i servizi al seggio
    runServicesToSeggio();
    // il thread che eseguiva la funzione termina se la funzione arriva alla fine

    server_thread.join();
    return;
}

void PostazioneVoto::function_thread_sendStatoToSeggio(unsigned int statoPV){
    //---bisogna comunicare alla postazione seggio che lo stato della postazione di voto X è cambiato---
    //iniziare una sessione ssl con la postazione di voto
    this->mutex_stdout.lock();
    cout << "Dentro il thread" << endl;
    this->mutex_stdout.unlock();

    const char * postazioneSeggio = "192.168.192.130"; //ricavare l'IP della postazione seggio a cui la postazione voto appartiene1

    SSLClient * pv_client = new SSLClient(this);

    if(pv_client->connectTo(postazioneSeggio)!=nullptr){

        pv_client->updateStatoPVtoSeggio(this->idPostazioneVoto,statoPV);
    }
    else{
        cerr << "PV: Postazione Seggio non raggiungibile, non è possibile inviare l'aggiornamento di stato" << endl;
    }


    delete pv_client;
}

void PostazioneVoto::setIdProceduraVoto(uint idProcedura){
    this->idProceduraVoto = idProcedura;
}

string PostazioneVoto::calcolaMAC(string key, string plainText){
    //AutoSeededRandomPool prng;

    //    SecByteBlock key(16);
    //    prng.GenerateBlock(key, key.size());

    string encodedKey = key;//"11A47EC4465DD95FCD393075E7D3C4EB";

    // Pretty print key
    //    encodedKey.clear();
    //    StringSource ss1(decodedKey, decodedKey.size(), true,
    //        new HexEncoder(
    //            new StringSink(encodedKey)
    //        ) // HexEncoder
    //    ); // StringSource

    cout << "key: " << key << endl;
    string decodedKey;

    StringSource ss(encodedKey,
                    new HexDecoder(
                        new StringSink(decodedKey)
                        ) // HexDecoder
                    ); // StringSource

    string plain = plainText;//"HMAC Test";
    string mac, encodedMAC;

    /*********************************\
    \*********************************/


    cout << "plain text: " << plain << endl;

    /*********************************\
    \*********************************/


    //generazione dell'hmac
    try
    {
        //dichiarazione del filtro
        SecByteBlock key(reinterpret_cast<const byte*>(decodedKey.data()), decodedKey.size());
        CryptoPP::HMAC< CryptoPP::SHA256 > hmac(key, key.size());

        StringSource ss2(plain, true,
                         new HashFilter(hmac,
                                        new StringSink(mac)
                                        ) // HashFilter
                         ); // StringSource
    }
    catch(const CryptoPP::Exception& e)
    {
        cerr << e.what() << endl;
        exit(1);
    }

    /*********************************\
    \*********************************/

    // Pretty print
    encodedMAC.clear();
    StringSource ss3(mac, true,
                     new HexEncoder(
                         new StringSink(encodedMAC)
                         ) // HexEncoder
                     ); // StringSource

    cout << "hmac: " << encodedMAC << endl;

    //qui encoded contiene l'hmac
    return encodedMAC;
}

void PostazioneVoto::addScheda(string scheda)
{
    SchedaVoto sv;

    //TODO parsing file xml e inserimento dati nell'oggetto scheda voto da aggiungere al vettore delle schede

    XMLDocument xmlDoc;
    xmlDoc.Parse(scheda.c_str());

    XMLNode *rootNode = xmlDoc.FirstChild();

    XMLText* textNodeIdProcedura = rootNode->FirstChildElement("idProcedura")->FirstChild()->ToText();
    uint idProcedura = atoi(textNodeIdProcedura->Value());
    cout << "idProcedura: " << idProcedura << endl;
    sv.setIdProceduraVoto(idProcedura);

    XMLText* textNodeIdScheda = rootNode->FirstChildElement("id")->FirstChild()->ToText();
    uint idScheda = atoi(textNodeIdScheda->Value());
    cout << "idScheda: " << idScheda << endl;
    sv.setId(idScheda);

    XMLText* textNodeTipologiaElezione= rootNode->FirstChildElement("tipologiaElezione")->FirstChild()->ToText();
    uint tipologiaElezione = atoi(textNodeTipologiaElezione->Value());
    cout << "tipologia elezione: " << tipologiaElezione << endl;
    sv.setTipoElezione(tipologiaElezione);

    XMLText* textNodeNumeroPreferenze = rootNode->FirstChildElement("numeroPreferenze")->FirstChild()->ToText();
    uint numeroPreferenze = atoi(textNodeNumeroPreferenze->Value());
    cout << "Numero preferenze: " << numeroPreferenze << endl;
    sv.setNumPreferenze(numeroPreferenze);


    XMLElement * listeElement = rootNode->FirstChildElement("liste");

    XMLElement * firstListaElement = listeElement->FirstChildElement("lista");
    XMLElement * lastListaElement = listeElement->LastChildElement("lista");

    XMLElement *listaElement = firstListaElement;
    bool lastLista = false;
    do{

        int idLista = listaElement->IntAttribute("id");
        cout <<" --- lista trovata" << endl;
        cout << "id Lista: " << idLista << endl;
        string nomeLista = listaElement->Attribute("nome");
        cout << "nome: " << nomeLista << endl;

        XMLElement * firstCandidatoElement  = listaElement->FirstChildElement("candidato");
        XMLElement * lastCandidatoElement = listaElement->LastChildElement("candidato");

        XMLElement * candidatoElement = firstCandidatoElement;
        //ottengo tutti i candidati della lista
        bool lastCandidato = false;
        do{
            int id = candidatoElement->IntAttribute("id");
            cout << "trovato candidato, id: " << id << endl;

            XMLElement * matricolaElement = candidatoElement->FirstChildElement("matricola");
            XMLNode * matricolaInnerNode = matricolaElement->FirstChild();
            string matricola;
            if(matricolaInnerNode!=nullptr){
                matricola = matricolaInnerNode->ToText()->Value();
            }
            cout << matricola << endl;

            XMLElement *nomeElement = matricolaElement->NextSiblingElement("nome");
            XMLNode * nomeInnerNode = nomeElement->FirstChild();
            string nome;
            if(nomeInnerNode!=nullptr){
                nome = nomeInnerNode->ToText()->Value();
            }
            cout << nome << endl;

            XMLElement *cognomeElement = nomeElement->NextSiblingElement("cognome");
            XMLNode * cognomeInnerNode = cognomeElement->FirstChild();
            string cognome;
            if(cognomeInnerNode!=nullptr){
                cognome = cognomeInnerNode->ToText()->Value();
            }
            cout << cognome << endl;

            XMLElement *luogoNascitaElement = cognomeElement->NextSiblingElement("luogoNascita");
            XMLNode * luogoNascitaInnerNode = luogoNascitaElement->FirstChild();
            string luogoNascita;
            if(luogoNascitaInnerNode!=nullptr){
                luogoNascita = luogoNascitaInnerNode->ToText()->Value();
            }
            cout << luogoNascita << endl;

            XMLElement *dataNascitaElement = luogoNascitaElement->NextSiblingElement("dataNascita");
            XMLNode * dataNascitaInnerNode = dataNascitaElement->FirstChild();
            string dataNascita;
            if(dataNascitaInnerNode!=nullptr){
                dataNascita = dataNascitaInnerNode->ToText()->Value();
            }
            cout << dataNascita << endl;

            cout << "Estratti i dati del candidato id: " << id << endl;
            sv.addCandidato(matricola,nome,cognome,nomeLista,dataNascita,luogoNascita);

            //accesso al successivo candidato
            if(candidatoElement == lastCandidatoElement){
                lastCandidato = true;
            }else {
                candidatoElement = candidatoElement->NextSiblingElement("candidato");
                cout << "ottengo il puntatore al successivo candidato" << endl;
            }
        }while(!lastCandidato);

        cout << "non ci sono altri candidati nella lista: " << nomeLista << endl;


        if(listaElement == lastListaElement){
            lastLista = true;
        }
        else{
            //accediamo alla successiva lista nella scheda di voto
            listaElement = listaElement->NextSiblingElement("lista");
            cout << "ottengo il puntatore alla successiva lista" << endl;
        }
    }while(!lastLista);
    cout << "non ci sono altre liste" << endl;

    schedeVoto.push_back(sv);
}

string PostazioneVoto::getSessionKey_PV_Urna() const
{
    return sessionKey_PV_Urna;
}

void PostazioneVoto::setSessionKey_PV_Urna(const string &value)
{
    sessionKey_PV_Urna = value;
}
