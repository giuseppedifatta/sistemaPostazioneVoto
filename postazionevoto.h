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
#include <unistd.h>
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

#include "sslclient.h"
#include "sslserver.h"
#include "mainwindowpv.h"
#include "schedavoto.h"
#include "schedacompilata.h"


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
    void wrongOTP();
    void giveNumberPV(uint numeroPostazione);
    //void urnaNonRaggiungibile();

public slots:

    void stopServerPV();
    void selectSchedeDaMostrare();
    //void inviaVotiToUrna(vector <SchedaCompilata> schede);
    void validatePassKey(QString pass);
    void validateOTP(QString otp);
    void numberPV();
    void inviaVotiToUrna2(vector<SchedaCompilata> schede);
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

    bool setHTAssociato(string tokenCod, string username, string password);
    void resetHT();
    void backToPostazioneAttiva();
    void setIdProceduraVoto(uint idProcedura);
    string calcolaMAC(string encodedSessionKey, string plainText);
    void addScheda(string scheda);
    void clearVectorSchede();
    string getSessionKey_PV_Urna() const;
    void setSessionKey_PV_Urna(const string &value);

    //string getPublicKeyRP() const;
    void setRSAPublicKeyRP(const string &publicKeyEncoded);



    bool tryConnectUrna();
    uint getMatricolaVotante() const;
    void setMatricolaVotante(const uint &value);

    int verifyMAC(string encodedSessionKey, string data, string macEncoded);
    string getUsernameHTAssociato() const;
    void setUsernameHTAssociato(const string &value);

    string getPasswordHTAssociato() const;
    void setPasswordHTAssociato(const string &value);

    uint getIdTipoVotante() const;
    void setIdTipoVotante(const uint &value);

    uint getIdSeggio() const;
    void setIdSeggio(const uint &value);

    string getIPbyInterface(const char *interfaceName);
private:
    bool attivata;
    string ipUrna;
    string postazioneSeggio;
    uint idSeggio;
    //dati membro

    unsigned int idPostazioneVoto; //relativo all'IP, da calcolare leggendo l'indirizzo IP del "localhost"
    string sessionKey_PV_Urna; //chiave di sessione handwritten al momento dell'attivazione della postazione di voto
    //string publicKeyRP;
    CryptoPP::RSA::PublicKey rsaPublicKeyRP;//ottenuta dall'Urna, chiave RSA per cifrare chiavi simmetriche in modo asimmetrico

    unsigned int idProceduraVoto;
    vector <SchedaVoto> schedeVoto;


    //informazioni da ricevere dal seggio, nel momento in cui effettua il push dell'associazione
    string usernameHTAssociato;
    string passwordHTAssociato;
    uint matricolaVotante;
    uint idTipoVotante;


    //vector <SchedaCompilata> schedeDaInviare;
    //unsigned int timeout;
    string HTAssociato; // non assegnato all'atto dell'inizializzazione

    statiPV statoPV;

    //SSLClient *pv_client;
    SSLServer *pv_server;

    std::thread server_thread;
    void runServerListenSeggio();

    //funzione per il thread client che deve inviare il cambio di stato alla postazione seggio
    void function_thread_sendStatoToSeggio(unsigned int statoPV);

    bool runServerPV;
    void creaSchedaCompilataXML_AES(XMLDocument *xmlDoc, SchedaCompilata scheda, SecByteBlock key, SecByteBlock iv);
    string AESencryptStdString(string plain, SecByteBlock key, SecByteBlock iv);
    string RSAencryptSecByteBlock(SecByteBlock valueBlock, CryptoPP::RSA::PublicKey publicKey);
    string calcolaIpSeggio(string ipPostazione);
    uint getIdPVbyMyIP(string myIP);
protected:

     void inactivitySessionClose();

    //std::string getStatoPostazioneAsString();

    bool voteAuthorizationWithOTP();

    string getHTAssociato();

    unsigned int getIdPostazioneVoto();

    //metodi per la cifratura del voto

    void pubEncrypt_publicKeyRP(unsigned int symKey, unsigned int iv);


};

#endif /* POSTAZIONEVOTO_H_ */
