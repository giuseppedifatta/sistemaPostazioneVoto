/*
 * PostazioneVoto.cpp
 *
 *  Created on: 15/gen/2017
 *      Author: giuseppe
 */

#include "postazionevoto.h"
int verifyMAC(string encodedSessionKey,string data, string macEncoded);
PostazioneVoto::PostazioneVoto(QObject *parent) :
    QThread(parent){
    //mainWindow = m;
    HTAssociato = 0;
    ivCBC = 0;
    symKeyAES = 0;
    attivata = false;

    //TODO calcolare dall'indirizzo IP
    idPostazioneVoto = 1;

    postazioneSeggio = "192.168.56.100"; //TODO ricavare l'IP della postazione seggio a cui la postazione voto appartiene
    ipUrna = "192.168.19.130";
    //init client
    //this->pv_client = new SSLClient(this);

    //abilito l'avvio del server in ascolto
    mutex_run_server.lock();
    this->runServerPV = true;
    mutex_run_server.unlock();
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

    //dovrei usare un mutex
    mutex_statoPV.lock();
    this->statoPV = nuovoStato;
    if(nuovoStato==statiPV::libera){
        attivata = true;
    }
    mutex_statoPV.unlock();

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

    //this->setStatoPV(statiPV::votazione_in_corso);
    emit giveSchedeToView(schedeDaMostrare);
}

//void PostazioneVoto::inviaVotiToUrna(vector<SchedaCompilata> schede)
//{
//    // con i dati della schede compilate, per ogni scheda crea un file xml
//    //di cui cifrare i campi contenenti i candidati votati e
//    //quindi invia uno per volta i file all'urna



//    //TODO comunico all'urna che un certo votante identificato da una certa matricola ha espresso il suo voto
//    SSLClient * pv_client1 = new SSLClient(this);
//    bool matricolaSettedVoted = false;
//    if(pv_client1->connectTo(ipUrna)!=nullptr){

//        if(pv_client1->setVoted(matricolaVotante)){
//            cout << "l'urna ha registrato che " << matricolaVotante << " ha votato" << endl;
//            matricolaSettedVoted = true;
//        }
//        else{
//            cerr << "impossibile comunicare esito della votazione all'urna" << endl;
//        }

//    }
//    else{
//        emit urnaNonRaggiungibile();
//        this->setStatoPV(statiPV::offline);
//        delete pv_client1;
//        //interrompiamo l'esecuzione della funzione, poichè non è possibile comunicare con l'urna e non sarebbe possibile inviare le schede di voto
//        //le schede di voto, sono attualmente salvate nel vettore delle schedeDaInviare
//        return;
//    }
//    delete pv_client1;

//    if(!matricolaSettedVoted){
//        cerr << "Errore nella registrazione del voto per la matricola: " << matricolaVotante << endl;
//        this->setStatoPV(statiPV::errore);
//        return;
//    }

//    //salvo temporaneamente le schede sulla postazione di voto
//    for (uint i = 0; i< schede.size(); i++){
//        schedeDaInviare.push_back(schede.at(i));
//    }
//    cout << "Schede da inviare: " << schedeDaInviare.size() << endl;

//    for(uint i = 0; i < schedeDaInviare.size(); i++){
//        cout << "Schede pendenti: " << schedeDaInviare.size() << endl;
//        bool schedaStored = false;
//        //generazione chiave simmetrica e iv
//        AutoSeededRandomPool rng;

//        // chiave simmetrica
//        SecByteBlock key( AES::MAX_KEYLENGTH );
//        rng.GenerateBlock( key, key.size() );


//        //initial value
//        SecByteBlock iv(AES::MAX_KEYLENGTH);
//        rng.GenerateBlock( iv, iv.size() );

//        //creazione file xml della scheda compilata e cifratura campi scheda voto con chiave simmetrica
//        XMLDocument xmlDoc;
//        creaSchedaCompilataXML_AES(&xmlDoc,schedeDaInviare.at(i),key,iv);

//        //cifratura chiave simmetrica e iv con chiave pubblica di RP
//        cout << "cifro Key e IV " << endl;
//        string encryptedKey = encryptRSA_withPublickKeyRP(key);
//        cout << "encrypted key: " << encryptedKey << endl;
//        string encryptedIV = encryptRSA_withPublickKeyRP(iv);
//        cout << "encrypted IV: " << encryptedIV << endl;


//        //        string encryptedKey = std::string(reinterpret_cast<const char*>(key.data()), key.size());
//        //        string encryptedIV = std::string(reinterpret_cast<const char*>(iv.data()), iv.size());

//        bool urnaUnreachable = false;
//        while (!schedaStored && !urnaUnreachable){

//            //generazione nonce

//            Integer randomUint(rng,32);
//            std::stringstream ss;
//            ss << randomUint;
//            std::string s(ss.str());
//            uint nonce = atoi(s.c_str());

//            cout << "nonce: " << nonce << endl;

//            //cifratura nonce
//            string encryptedNonce = encryptStdString(std::to_string(nonce),key,iv);

//            //sostituzione nonce nel file xml
//            XMLNode *rootNode = xmlDoc.FirstChild();

//            rootNode->FirstChildElement("nonce")->SetText(encryptedNonce.c_str());


//            //print file xml della scheda to string
//            XMLPrinter printer;
//            xmlDoc.Print( &printer );
//            string schedaStr = printer.CStr();
//            cout << schedaStr << endl;

//            //generazione mac
//            //dati di ingresso HMAC: scheda voto con campi candidati cifrati, chiave simmetrica e iv cifrati, nonce generato al passo precedente
//            //chiave per HAMC: chiave di sessione tra pv e urna
//            string datiConcatenati = schedaStr + encryptedKey + encryptedIV + std::to_string(nonce);
//            cout << "Dati di cui calcolare il mac: " << datiConcatenati << endl;

//            string macPacchettoVoto = calcolaMAC(sessionKey_PV_Urna,datiConcatenati);

//            //invio pacchetto di voto all'urna
//            SSLClient * pv_client = new SSLClient(this);

//            if(pv_client->connectTo(ipUrna)!=nullptr){
//                QThread::msleep(100);
//                if(pv_client->inviaSchedaCompilata(schedaStr,encryptedKey, encryptedIV,std::to_string(nonce),macPacchettoVoto)){

//                    //se il mac ricevuto dall'urna è univoco rispetto al db,
//                    //la memorizzazione del voto andrà a buon fine
//                    //settiamo schedaStored a true

//                    schedaStored = true;
//                    //eliminiamo la schedaCorrente dal vettore delle schede ancora da inviare all'urna
//                    schedeDaInviare.erase(schedeDaInviare.begin()+i);
//                    //la dimensione del vettore è diminuita di uno, il prossimo elemento da leggere si
//                    //trova una posizione indietro rispetto a prima di eseguire l'operazione appena precedente
//                    i--;
//                }
//                else{
//                    cerr << "scheda non memorizzata, verrà fatto un nuovo tentativo, cambiando l'nonce" << endl;
//                }
//            }
//            else{
//                emit urnaNonRaggiungibile();
//                this->setStatoPV(statiPV::offline);
//                urnaUnreachable = true;
//                delete pv_client;
//                //interrompiamo l'esecuzione della funzione, poichè non è possibile comunicare con l'urna
//                //e non sarebbe possibile inviare le schede di voto non ancora trasmesse
//                //le schede di voto restanti sono attualmente salvate nel vettore delle schedeDaInviare
//                return;
//            }
//            delete pv_client;
//        }
//    }


//    //tutte le schede votate sono state recapitate correttamente nell'urna

//    cout << "tutte le schede sono state consegnate all'urna virtuale" << endl;
//    //emettiamo il segnale per la view, così da comunicare all'elettore la conclusione corretta dell'operazione di voto
//    setStatoPV(statiPV::votazione_completata);
//}

void PostazioneVoto::inviaVotiToUrna2(vector<SchedaCompilata> schede){
    // con i dati della schede compilate, per ogni scheda crea un file xml
    //di cui cifrare i campi contenenti i candidati votati e
    //quindi invia uno per volta i file all'urna

    bool inviati = false;
    bool erroreInvio = false;
    bool postazioneOffline = false;

    SSLClient * pv_client = new SSLClient(this);

    //mi collego all'urna
    if(pv_client->connectTo(ipUrna)!=nullptr){

        uint numSchede = schede.size();
        cout << "Schede da inviare: " << numSchede << endl;

        //1. richiedo il servizio di invio voti
        pv_client->richiestaServizioInvioSchede(numSchede);


        //per ogni scheda eseguo l'invio

        for(uint i = 0; i < schede.size(); i++){


            //generazione chiave simmetrica e iv
            AutoSeededRandomPool rng;

            // chiave simmetrica
            SecByteBlock key( AES::MAX_KEYLENGTH ); //32 bytes
            rng.GenerateBlock( key, key.size() );


            //initial value
            SecByteBlock iv(AES::BLOCKSIZE);//16 bytes
            rng.GenerateBlock( iv, iv.size() );

            //creazione file xml della scheda compilata e cifratura campi scheda voto con chiave simmetrica
            XMLDocument xmlDoc;
            creaSchedaCompilataXML_AES(&xmlDoc,schede.at(i),key,iv);

            //cifratura chiave simmetrica e iv con chiave pubblica di RP
            cout << "cifro Key e IV " << endl;
            string encryptedKey = RSAencryptSecByteBlock(key,rsaPublicKeyRP);
            cout << "encrypted key: " << encryptedKey << endl;
            string encryptedIV = RSAencryptSecByteBlock(iv,rsaPublicKeyRP);
            cout << "encrypted IV: " << encryptedIV << endl;

            //2. invio delle chiavi di cifratura della scheda
            pv_client->invioChiavi(encryptedKey, encryptedIV);

            bool schedaStored = false;
            //3. invio scheda
            while (!schedaStored){
                cout << "invio scheda: " << i+1 << endl;
                //generazione nonce

                Integer randomUint(rng,32);
                std::stringstream ss;
                ss << randomUint;
                std::string s(ss.str());
                uint nonce = atoi(s.c_str());

                cout << "nonce: " << nonce << endl;

                //cifratura nonce
                string encryptedNonce = AESencryptStdString(std::to_string(nonce),key,iv);

                //aggiungo o sostituisco nonce nel file xml
                XMLNode *rootNode = xmlDoc.FirstChild();

                //il campo nonce, in fase di creazione scheda viene lasciato vuoto, qui inseriamo il valore
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
                cout << "Dati di cui calcolare il mac: " << datiConcatenati << endl;

                string macPacchettoVoto = calcolaMAC(sessionKey_PV_Urna,datiConcatenati);



                if(pv_client->inviaSchedaWithoutKeys(schedaStr,std::to_string(nonce),macPacchettoVoto)){

                    //se il mac ricevuto dall'urna è univoco rispetto al db,
                    //la memorizzazione del voto andrà a buon fine
                    //settiamo schedaStored a true

                    schedaStored = true;

                }
                else{
                    cerr << "scheda non memorizzata, verrà fatto un nuovo tentativo, cambiando l'nonce" << endl;
                }
            }//while

        }

        //comunicazione matricola e conferma esito positivo di ricezione schede

        if(pv_client->sendMatricolaAndConfirmStored(matricolaVotante)){
            inviati = true;
        }
        else{
            erroreInvio = true;
        }
    }//if connect


    else{ //else connect
        emit urnaNonRaggiungibile();

        postazioneOffline = true;
        //interrompiamo l'esecuzione della funzione, poichè non è possibile comunicare con l'urna e non sarebbe possibile inviare le schede di voto
        //le schede di voto, sono attualmente salvate nel vettore delle schedeDaInviare

    }
    delete pv_client;




    cout << "tutte le schede sono state consegnate all'urna virtuale" << endl;
    //settiamo lo stato della postazione in base all'esito dell'operazione
    if(inviati){
        //tutte le schede votate sono state recapitate correttamente nell'urna
        cout << "TUTTE LE SCHEDE INVIATE!!!" << endl;
        setStatoPV(statiPV::votazione_completata);
        return;
    }
    if(postazioneOffline){

        setStatoPV(statiPV::offline);
        return;
    }
    if(erroreInvio){
        setStatoPV(statiPV::errore);
        return;
    }
}

void PostazioneVoto::tryConnectUrna()
{
    SSLClient * pv_client = new SSLClient(this);

    if(pv_client->connectTo(ipUrna)!=nullptr){


        pv_client->sendCodConnection();
        if(attivata){
            setStatoPV(statiPV::libera);
        }
        else{
            setStatoPV(statiPV::attesa_attivazione);
        }
    }
    else{
        emit urnaNonRaggiungibile();
    }
    delete pv_client;
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
        string matricolaEncrypted = AESencryptStdString(matricola,key,iv);
        pMatr->SetText(matricolaEncrypted.c_str());
        pPreferenze->InsertEndChild(pMatr);
    }
}

string PostazioneVoto::AESencryptStdString(string plain, SecByteBlock key, SecByteBlock  iv) {


    string cipher, encoded;

    /*********************************\
    \*********************************/

    // Pretty print key
    encoded.clear();
    StringSource(key, key.size(), true,
                 new HexEncoder(
                     new StringSink(encoded)
                     ) // HexEncoder
                 ); // StringSource
    cout << "key: " << encoded << endl;

    // Pretty print iv
    encoded.clear();
    StringSource(iv,iv.size(),true,
                 new HexEncoder(
                     new StringSink(encoded)
                     ) // HexEncoder
                 ); // StringSource
    cout << "iv: " << encoded << endl;

    /*********************************\
    \*********************************/

    try
    {
        cout << "plain text: " << plain << endl;

        CBC_Mode< AES >::Encryption aesEncryptor;
        aesEncryptor.SetKeyWithIV(key, key.size(), iv);

        // The StreamTransformationFilter removes
        //  padding as required.
        StringSource (plain, true,
                      new StreamTransformationFilter(aesEncryptor,
                                                     new StringSink(cipher)
                                                     ) // StreamTransformationFilter
                      ); // StringSource


    }
    catch(const CryptoPP::Exception& e)
    {
        cerr << "Caught exception :" << e.what() << endl;
    }

    /*********************************\
    \*********************************/

    // Pretty print
    string encodedCipher;
    StringSource(cipher, true,
                 new HexEncoder(
                     new StringSink(encodedCipher)
                     ) // HexEncoder
                 ); // StringSource
    cout << "cipher text: " << encodedCipher << endl;

    return encodedCipher;
}

string PostazioneVoto::RSAencryptSecByteBlock(SecByteBlock valueBlock,CryptoPP::RSA::PublicKey publicKey)
{
    std::string plain = std::string(reinterpret_cast<const char*>(valueBlock.data()), valueBlock.size());
    AutoSeededRandomPool rng;

    string cipher;
    cout << "plain: " << plain << endl;

    try{
    ////////////////////////////////////////////////
    // Encryption // con la chiave pubblica di RP
    RSAES_OAEP_SHA_Encryptor rsaEncryptor( publicKey );

    StringSource( plain, true,
                  new PK_EncryptorFilter( rng, rsaEncryptor,
                                          new StringSink( cipher )
                                          ) // PK_EncryptorFilter
                  ); // StringSource

    cout << "cipher:" << cipher << endl;
    }
    catch(const CryptoPP::Exception& e)
    {
        cerr << "Caught exception :" << e.what() << endl;
    }
    string encodedCipher;
    StringSource(cipher,true,
                 new HexEncoder(
                     new StringSink(encodedCipher)
                     )//HexEncoder
                 );//StringSource
    cout << "encoded cipher: " << encodedCipher << endl;


    // Encryption

    return encodedCipher;
}

void PostazioneVoto::validatePassKey(QString pass)
{
    //contatto l'urna per validare la password


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
    else{
        emit urnaNonRaggiungibile();
        this->setStatoPV(statiPV::offline);
    }

    delete pv_client;
}

void PostazioneVoto::validateOTP(QString otp)
{

    //TODO contattare otpServer per verificare il token rispetto all'account relativo al token associato alla postazione voto
    if(otp=="123456"){
        enablingPV();
    }
    else{
        emit wrongOTP();
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

string PostazioneVoto::calcolaMAC(string encodedSessionKey, string plain){


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

    cout << "plain text: " << plain << endl;

    /*********************************\
    \*********************************/

    try
    {
        CryptoPP::HMAC< CryptoPP::SHA256 > hmac(key, key.size());

        StringSource(plain, true,
                     new HashFilter(hmac,
                                    new StringSink(macCalculated)
                                    ) // HashFilter
                     ); // StringSource
    }
    catch(const CryptoPP::Exception& e)
    {
        cerr << "Caught exception :" << e.what() << endl;
    }

    /*********************************\
    \*********************************/

    //	// Pretty print MAC
    string macEncoded;
    StringSource(macCalculated, true,
                 new HexEncoder(
                     new StringSink(macEncoded)
                     ) // HexEncoder
                 ); // StringSource
    cout << "hmac encoded: " << macEncoded << endl;

    return macEncoded;
}

int PostazioneVoto::verifyMAC(string encodedSessionKey,string data, string macEncoded){
    //restituisce 0 in caso di verifica positiva
    //restituisce 1 in caso di verifica negativa
    string decodedKey;
    int success = 1;
    cout << "Session key: " << encodedSessionKey << endl;

    StringSource (encodedSessionKey,true,
                  new HexDecoder(
                      new StringSink(decodedKey)
                      ) // HexDecoder
                  ); // StringSource

    SecByteBlock key(reinterpret_cast<const byte*>(decodedKey.data()), decodedKey.size());

    string macDecoded;
    StringSource(macEncoded, true,
                 new HexDecoder(
                     new StringSink(macDecoded)
                     ) // HexEncoder
                 ); // StringSource
    cout << "hmac decoded: " << macDecoded << endl;

    try
    {
        CryptoPP::HMAC< CryptoPP::SHA256 > hmac(key, key.size());
        const int flags = HashVerificationFilter::THROW_EXCEPTION | HashVerificationFilter::HASH_AT_END;


        StringSource(data + macDecoded, true,
                     new HashVerificationFilter(hmac, NULL, flags)
                     ); // StringSource
        success = 0;
        cout << "Verified message" << endl;
    }
    catch(const CryptoPP::Exception& e)
    {
        cerr << "Caught exception :" << e.what() << endl;
    }
    return success;
}

void PostazioneVoto::addScheda(string scheda)
{
    SchedaVoto sv;

    //parsing file xml e inserimento dati nell'oggetto scheda voto da aggiungere al vettore delle schede

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



void PostazioneVoto::setRSAPublicKeyRP(const string &publicKeyEncoded)
{
    string decodedPublicKey;

    StringSource (publicKeyEncoded, true /*pump all*/,
                  new HexDecoder(
                      new StringSink(decodedPublicKey)
                      ) // HexDecoder
                  ); // StringSource
    cout << "publicKey decodedificata da esadecimale: " << decodedPublicKey << endl;

    StringSource ss(decodedPublicKey,true /*pumpAll*/);
    rsaPublicKeyRP.Load(ss);

    cout << "PV: la chiave publica di RP è stata salvata sulla postazione" << endl;
}

unsigned int PostazioneVoto::getTipoElettore() const
{
    return tipoElettore;
}

void PostazioneVoto::setTipoElettore(unsigned int value)
{
    tipoElettore = value;
}


uint PostazioneVoto::getMatricolaVotante() const
{
    return matricolaVotante;
}

void PostazioneVoto::setMatricolaVotante(const uint &value)
{
    matricolaVotante = value;
}
