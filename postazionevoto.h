/*
 * PostazioneVoto.h
 *
 *  Created on: 15/gen/2017
 *      Author: giuseppe
 */

#ifndef POSTAZIONEVOTO_H_
#define POSTAZIONEVOTO_H_

#include <tinyxml2.h>
#include <string>
#include <mutex>
#include <thread>
#include "sslclient.h"
#include "sslserver.h"
#include "mainwindowpv.h"

#include <QtCore>
#include <QThread>

using namespace std;
using namespace tinyxml2;

class SSLClient;
class SSLServer;
class MainWindowPV;

class PostazioneVoto : public QThread
{
    Q_OBJECT

signals:
    void stateChange(unsigned int);

public slots:
    void validatePassKey(QString pass);
    void stopServerPV();


public:
    explicit PostazioneVoto(QObject *parent = 0);
    virtual ~PostazioneVoto();

    //funzione eseguita quando viene avviata l'esecuzione del thread
    void run();

    enum statiPV {
        attesa_attivazione,
        libera,
        attesa_abilitazione,
        votazione_in_corso,
        votazione_completata,
        errore,
        offline
    };
    void setStatoPV(statiPV nuovoStato);
    unsigned int getStatoPV();

    //metodi per la visualizzazione delle schermate
    void mostraSchede(XMLDocument *pschedeVoto);


    //comunica con l'otp server provider per fornire il codice inserito, restituisce true, se il codice inserito è esatto
    bool enablingPV();
    void runServicesToSeggio();



    mutex mutex_stdout;
    QMutex mutex_run_server;

    bool setHTAssociato(unsigned int tokenCod);
    void backToPostazioneAttiva();

private:

    //dati membro
    //MainWindowPV *mainWindow;
    unsigned int idPostazioneVoto; //relativo all'IP, da calcolare leggendo l'indirizzo IP del "localhost"
    unsigned int sessionKey_PV_Urna; //chiave privata presente sulla smart card
    unsigned int publicKeyPV; //prelevato dalla SC all'atto dell'inizializzazione
    unsigned int publicKeyRP; //ottenuto dall'Urna

    unsigned short int idProceduraVoto;
    tinyxml2::XMLDocument * pSchedeVoto;

    //unsigned int timeout;
    unsigned int HTAssociato; // non assegnato all'atto dell'inizializzazione
    unsigned int symKeyAES; //chiave per la cifratura simmetrica
    unsigned int ivCBC; //valore iniziale per CBC

    statiPV statoPV;

    SSLClient *pv_client;
    SSLServer *pv_server;

    std::thread server_thread;
    void runServerListenSeggio();

    bool runServerPV;
protected:

    //monitoraggio postazione, servizi per il seggio
    bool feedbackFreeBusy();
    bool offlinePV();


    void inactivitySessionClose();

    //std::string getStatoPostazioneAsString();

    bool voteAuthorizationWithOTP();

    unsigned int getHTAssociato();

    unsigned int getIdPostazioneVoto();
    unsigned int getPublicKeyPV(); //servizio per l'urna, inutile se le SmartCard delle postazioni voto sono prememorizzate nell'urna, se la PV invia l'id relativo alla SC che è inserita
    void compilaScheda(); //estrae i dati dalla schermata di compilazione di una singola scheda e inserisce un elemento nel vettore delle schedeCompilate



    //metodi per la cifratura del voto

    bool inviaSchedeToUrnaVirtuale(); //passare le schedecompilate come parametro
    void pubEncrypt_publicKeyRP(unsigned int symKey, unsigned int iv);

    //calcola il digest sui dati in ingresso, lo firma(cio� lo cifra con la chiave privata), restituisce il digest firmato
    //il tipo di valore di ritorno dipende dal tipo di ritorno del metodo di firma usato
    string firmaVC_PV(unsigned int symKey, unsigned int iv,
                      XMLDocument schedaVotoCompilataCifrata);

    XMLDocument symEncrypt_V(XMLDocument schedaVoto);

};

#endif /* POSTAZIONEVOTO_H_ */
