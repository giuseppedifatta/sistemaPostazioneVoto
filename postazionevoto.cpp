/*
 * PostazioneVoto.cpp
 *
 *  Created on: 15/gen/2017
 *      Author: giuseppe
 */

#include "postazionevoto.h"

PostazioneVoto::PostazioneVoto(QObject *parent) :
    QThread(parent){
    //mainWindow = m;
    HTAssociato = 0;
    ivCBC = 0;
    symKeyAES = 0;

    //TODO calcolare dall'indirizzo IP
    idPostazioneVoto = 1;

    postazioneSeggio = "192.168.56.100"; //ricavare l'IP della postazione seggio a cui la postazione voto appartiene1

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

void PostazioneVoto::runServicesToSeggio() {

    server_thread = std::thread(&PostazioneVoto::runServerListenSeggio, this);

}

bool PostazioneVoto::enablingPV() {
    setStatoPV(statiPV::votazione_in_corso);
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

void PostazioneVoto::selectSchedeDaMostrare()
{
    vector <SchedaVoto> schedeDaMostrare;

    for (unsigned int i = 0; i < schedeVoto.size(); i++){
        //TODO seleziona la scheda se l'elettore corrente assegnato alla postazione può votare per questa scheda
        schedeDaMostrare.push_back(schedeVoto.at(i));
    }

    emit giveSchedeToView(schedeDaMostrare);
}

void PostazioneVoto::inviaVotiToUrna(vector<SchedaCompilata> schede)
{
    //TODO con i dati della schede compilate, creare un vettore di file xml
    //di cui cifrare i campi contenenti i candidati votati e
    //quindi inviare uno per volta i file all'urna

    for (uint i = 0; i < schede.size(); i++){
        bool schedaStored = false;
        //generazione chiave simmetrica e iv
        AutoSeededRandomPool rng;

        // chiave simmetrica
        SecByteBlock key( AES::MAX_KEYLENGTH );
        rng.GenerateBlock( key, key.size() );


        //initial value
        SecByteBlock iv(AES::MAX_KEYLENGTH);
        rng.GenerateBlock( iv, iv.size() );

        //creazione file xml della scheda compilata e cifratura campi scheda voto con chiave simmetrica
        XMLDocument xmlDoc;
        creaSchedaCompilataXML_AES(&xmlDoc,schede.at(i),key,iv);



        //TODO cifratura chiave simmetrica e iv con chiave pubblica di RP
        cout << "cifro Key e IV " << endl;
        string encryptedKey = encryptRSA_withPublickKeyRP(key);
        string encryptedIV = encryptRSA_withPublickKeyRP(iv);

        //        string encryptedKey = std::string(reinterpret_cast<const char*>(key.data()), key.size());
        //        string encryptedIV = std::string(reinterpret_cast<const char*>(iv.data()), iv.size());


        while (!schedaStored){
            //generazione nonce
            uint nonce = rng.GenerateBit();

            //cifratura nonce
            string encryptedNonce = encryptStdString(std::to_string(nonce),key,iv);

            //sostituzione nonce nel file xml
            XMLNode *rootNode = xmlDoc.FirstChild();

            rootNode->FirstChildElement("nonce")->SetText(encryptedNonce.c_str());


            //print file xml della scheda to string
            XMLPrinter printer;
            xmlDoc.Print( &printer );
            string schedaStr = printer.CStr();
            cout << schedaStr << endl;

            //generazione mac
            //dati di ingresso HMAC: scheda voto con campi candidati cifrati, chiave simmetrica e iv cifrati, nonce generato al passo precedente
            //chiave per HAMC: chiave di sessione tra pv e urna
            string datiConcatenati = schedaStr + encryptedKey + encryptedIV + std::to_string(nonce);

            string macPacchettoVoto = calcolaMAC(sessionKey_PV_Urna,datiConcatenati);

            //invio pacchetto di voto all'urna
            const char * ipUrna = "192.168.19.129";

            SSLClient * pv_client = new SSLClient(this);

            if(pv_client->connectTo(ipUrna)!=nullptr){

                if(pv_client->inviaSchedaCompilata(schedaStr,encryptedKey, encryptedIV,std::to_string(nonce),macPacchettoVoto)){

                    //se il mac ricevuto dall'urna è univoco rispetto al db,
                    //la memorizzazione del voto andrà a buon fine
                    //settiamo schedaStored a true

                    schedaStored = true;
                }
                else{
                    cerr << "scheda non memorizzata, verrà fatto un nuovo tentativo, cambiando l'nonce" << endl;
                }
            }

            delete pv_client;


        }
    }


    //tutte le schede votate sono state recapitate correttamente nell'urna
    //emettiamo il segnale per la view, così da comunicare all'elettore la conclusione corretta dell'operazione di voto
    setStatoPV(statiPV::votazione_completata);
}

void PostazioneVoto::creaSchedaCompilataXML_AES(XMLDocument  * xmlDoc, SchedaCompilata scheda, SecByteBlock key, SecByteBlock iv){
    // estrazione dati dall'oggetto di tipo scheda voto, e generazione del file XML, con conseguente memorizzazione su database


    XMLNode * pRoot = xmlDoc->NewElement("SchedaCompilata");
    xmlDoc->InsertFirstChild(pRoot);
    XMLElement * pElement;


    uint idScheda = scheda.getIdScheda();
    pElement = xmlDoc->NewElement("idScheda");
    pElement->SetText(idScheda);
    pRoot->InsertEndChild(pElement);

    uint idProceduraVoto = scheda.getIdProcedura();
    pElement = xmlDoc->NewElement("idProcedura");
    pElement->SetText(idProceduraVoto);
    pRoot->InsertEndChild(pElement);

    uint tipoElezione = scheda.getTipologiaElezione();
    pElement = xmlDoc->NewElement("tipologiaElezione");
    pElement->SetText(tipoElezione);
    pRoot->InsertEndChild(pElement);

    uint numPref = scheda.getNumPreferenze();
    pElement = xmlDoc->NewElement("numeroPreferenze");
    pElement->SetText(numPref);
    pRoot->InsertEndChild(pElement);

    pElement = xmlDoc->NewElement("nonce");
    pRoot->InsertEndChild(pElement);

    vector <string> preferenzeMatricole = scheda.getMatricolePreferenze();
    XMLNode * pPreferenze = xmlDoc->NewElement("preferenze");
    pRoot->InsertEndChild(pPreferenze);
    //    unsigned int idCandidato = 0;
    //    unsigned int idLista = 0;
    for (uint indexMatricole = 0; indexMatricole < preferenzeMatricole.size(); indexMatricole++){

        XMLElement * pMatr = xmlDoc->NewElement("matricolaCandidato");
        string matricola = preferenzeMatricole.at(indexMatricole);
        //cifriamo il campo della matricola con chiave simmetrica e AES
        string matricolaEncrypted = encryptStdString(matricola,key,iv);
        pMatr->SetText(matricolaEncrypted.c_str());
        pPreferenze->InsertEndChild(pMatr);
    }
}

string PostazioneVoto::encryptStdString(string plaintext, SecByteBlock key, SecByteBlock iv) {

    string ciphertext;
    //settaggio parametri di cifratura
    //impostazione della chiave che sarà utilizzata da AES
    CryptoPP::AES::Encryption aesEncryption(key,AES::MAX_KEYLENGTH);

    //impostazione dell'iv per il cifrario a blocchi
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv);

    //impostazione variabile di destinazione del testo cifrato
    StreamTransformationFilter stfEncryptor(cbcEncryption,new StringSink(ciphertext));
    //cifratura del plaintext
    stfEncryptor.Put(reinterpret_cast<const unsigned char*>(plaintext.c_str()),plaintext.length() + 1);
    stfEncryptor.MessageEnd();


    return ciphertext;
}

string PostazioneVoto::encryptRSA_withPublickKeyRP(SecByteBlock value)
{
    // Encrypt
    RSAES_OAEP_SHA_Encryptor encryptor(rsaPublicKeyRP);

    //Encrypt key
    // Now that there is a concrete object, we can validate
    assert(0 != encryptor.FixedMaxPlaintextLength());
    assert(
            CryptoPP::AES::DEFAULT_KEYLENGTH
                    <= encryptor.FixedMaxPlaintextLength());

    // Create cipher key space
    size_t ecl = encryptor.CiphertextLength(value.size());
    assert(0 != ecl);

    SecByteBlock cipherKey(ecl);

    // Paydirt
    AutoSeededRandomPool rng;
    encryptor.Encrypt(rng, value, value.size(), cipherKey);

    string encryptedValue = std::string(reinterpret_cast<const char*>(cipherKey.data()), cipherKey.size());

    return encryptedValue;
}

void PostazioneVoto::validatePassKey(QString pass)
{
    //contatto l'urna per validare la password
    const char * ipUrna = "192.168.19.129";

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

    delete pv_client;
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

string PostazioneVoto::calcolaMAC(string encodedSessionKey, string plainText){


    //"11A47EC4465DD95FCD393075E7D3C4EB";

    cout << "Session key: " << encodedSessionKey << endl;
    string decodedKey;
    StringSource (encodedSessionKey,true,
            new HexDecoder(
                    new StringSink(decodedKey)
            ) // HexDecoder
    ); // StringSource

    SecByteBlock key(reinterpret_cast<const byte*>(decodedKey.data()), decodedKey.size());


    string macCalculated, encoded;

    /*********************************\
    \*********************************/

    // Pretty print key
    encoded.clear();
    StringSource(key, key.size(), true,
        new HexEncoder(
            new StringSink(encoded)
        ) // HexEncoder
    ); // StringSource
    cout << "key encoded: " << encoded << endl;

    cout << "plain text: " << plainText << endl;

    /*********************************\
    \*********************************/

    try
    {
        CryptoPP::HMAC< CryptoPP::SHA256 > hmac(key, key.size());

        StringSource(plainText, true,
            new HashFilter(hmac,
                new StringSink(macCalculated)
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

    // Pretty print MAC
    string macEncoded;
    StringSource(macCalculated, true,
        new HexEncoder(
            new StringSink(macEncoded)
        ) // HexEncoder
    ); // StringSource
    cout << "hmac encoded: " << macEncoded << endl;


    return macEncoded;
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

//string PostazioneVoto::getPublicKeyRP() const
//{
//    return publicKeyRP;
//}

void PostazioneVoto::setRSAPublicKeyRP(const string &publicKeyEncoded)
{
    string decodedPublicKey;

    StringSource ssDecoded(publicKeyEncoded, true /*pump all*/,
                           new HexDecoder(
                               new StringSink(decodedPublicKey)
                               ) // HexDecoder
                           ); // StringSource
    cout << "publicKey decodedificata da esadecimale: " << decodedPublicKey << endl;

    StringSource ss(decodedPublicKey,true /*pumpAll*/);
    rsaPublicKeyRP.Load(ss);
}
