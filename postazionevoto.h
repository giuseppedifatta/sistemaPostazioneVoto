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
#include "sslclient.h"

using namespace std;
using namespace tinyxml2;

class PostazioneVoto {
public:
    PostazioneVoto();
    virtual ~PostazioneVoto();
    bool smartCardIsOn();
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

    //metodi per la visualizzazione delle schermate
    void mostraSchede(XMLDocument *pschedeVoto);
    bool mostraAuthenticationOTP();
    void mostraPostazioneVotoInizializzata();
    void mostraVotoMemorizzato();

    bool enablingPV();
    int servicesToSeggio();
    //dati membro
private:
    unsigned int idPostazioneVoto; //relativo all'IP
    unsigned int sessionKey_PV_Urna; //chiave privata presente sulla smart card
    unsigned int publicKeyPV; //prelevato dalla SC all'atto dell'inizializzazione
    unsigned int publicKeyRP; //ottenuto dall'Urna

    unsigned short int idProceduraVoto;
    tinyxml2::XMLDocument * pSchedeVoto;

    unsigned int timeout;
    unsigned int HTAssociato; // non assegnato all'atto dell'inizializzazione
    unsigned int symKeyAES; //chiave per la cifratura simmetrica
    unsigned int ivCBC; //valore iniziale per CBC

    statiPV statoPV;

    SSLClient *pv_client;

protected:

    //monitoraggio postazione, servizi per il seggio
    bool feedbackFreeBusy();
    void inactivitySessionClose();

    bool offlinePV();

    std::string getStatoPostazioneAsString();

    bool voteAuthorizationWithOTP();
    void setHTAssociato(unsigned int tokenCod);
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
