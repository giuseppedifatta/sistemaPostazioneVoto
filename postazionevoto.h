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
#include <sstream>

#include "sslclient.h"
#include "sslserver.h"
#include "mainwindowpv.h"
#include "schedavoto.h"
#include "schedacompilata.h"
#include <iostream>
#include "cryptopp/osrng.h"
#include "cryptopp/cryptlib.h"
#include "cryptopp/hmac.h"
#include "cryptopp/sha.h"
#include "cryptopp/hex.h"
#include "cryptopp/filters.h"
#include "cryptopp/secblock.h"
#include "cryptopp/aes.h"
#include "cryptopp/modes.h"
#include <cryptopp/rsa.h>



#include <QtCore>
#include <QThread>

using namespace std;
using namespace tinyxml2;
using namespace CryptoPP;

class SSLClient;
class SSLServer;
class MainWindowPV;

class PostazioneVoto : public QThread
{
    Q_OBJECT

signals:
    void stateChange(unsigned int);
    void wrongPassKey();
    void giveSchedeToView(vector <SchedaVoto> schedeDaMostrare);


public slots:

    void stopServerPV();
    void selectSchedeDaMostrare();
    void inviaVotiToUrna(vector <SchedaCompilata> schede);
    void validatePassKey(QString pass);
    void validateOTP(QString otp);

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

    //comunica con l'otp server provider per fornire il codice inserito, restituisce true, se il codice inserito è esatto
    bool enablingPV();
    void runServicesToSeggio();

    mutex mutex_stdout;
    mutex mutex_statoPV;
    QMutex mutex_run_server;

    bool setHTAssociato(unsigned int tokenCod);
    void resetHT();
    void backToPostazioneAttiva();
    void setIdProceduraVoto(uint idProcedura);
    string calcolaMAC(string encodedSessionKey, string plainText);
    void addScheda(string scheda);
    string getSessionKey_PV_Urna() const;
    void setSessionKey_PV_Urna(const string &value);

    //string getPublicKeyRP() const;
    void setRSAPublicKeyRP(const string &publicKeyEncoded);

private:
    const char * postazioneSeggio;
    //dati membro

    unsigned int idPostazioneVoto; //relativo all'IP, da calcolare leggendo l'indirizzo IP del "localhost"
    string sessionKey_PV_Urna; //chiave di sessione handwritten al momento dell'attivazione della postazione di voto
    //string publicKeyRP;
    CryptoPP::RSA::PublicKey rsaPublicKeyRP;//ottenuta dall'Urna, chiave RSA per cifrare chiavi simmetriche in modo asimmetrico

    unsigned int idProceduraVoto;
    vector <SchedaVoto> schedeVoto;
    unsigned int tipoElettore;//in funziona di questo valore vanno selezionate le schede da mostrare per la compilazione

    //unsigned int timeout;
    unsigned int HTAssociato; // non assegnato all'atto dell'inizializzazione
    unsigned int symKeyAES; //chiave per la cifratura simmetrica, non di tipo uint
    unsigned int ivCBC; //valore iniziale per CBC, non di tipo uint

    statiPV statoPV;

    //SSLClient *pv_client;
    SSLServer *pv_server;

    std::thread server_thread;
    void runServerListenSeggio();

    //funzione per il thread client che deve inviare il cambio di stato alla postazione seggio
    void function_thread_sendStatoToSeggio(unsigned int statoPV);

    bool runServerPV;
    void creaSchedaCompilataXML_AES(XMLDocument *xmlDoc, SchedaCompilata scheda, SecByteBlock key, SecByteBlock iv);
    string encryptStdString(string plaintext, SecByteBlock key, SecByteBlock iv);
    string encryptRSA_withPublickKeyRP(SecByteBlock value);
protected:

    //monitoraggio postazione, servizi per il seggio
    bool feedbackFreeBusy();
    bool offlinePV();


    void inactivitySessionClose();

    //std::string getStatoPostazioneAsString();

    bool voteAuthorizationWithOTP();

    unsigned int getHTAssociato();

    unsigned int getIdPostazioneVoto();



    //metodi per la cifratura del voto

    void pubEncrypt_publicKeyRP(unsigned int symKey, unsigned int iv);

    //calcola il digest sui dati in ingresso, lo firma(cio� lo cifra con la chiave privata), restituisce il digest firmato
    //il tipo di valore di ritorno dipende dal tipo di ritorno del metodo di firma usato
    string firmaVC_PV(unsigned int symKey, unsigned int iv,
                      XMLDocument schedaVotoCompilataCifrata);

    XMLDocument symEncrypt_V(XMLDocument schedaVoto);

};

#endif /* POSTAZIONEVOTO_H_ */
