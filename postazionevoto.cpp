/*
 * PostazioneVoto.cpp
 *
 *  Created on: 15/gen/2017
 *      Author: giuseppe
 */

#include "postazionevoto.h"
#include "openotp_login.h"


PostazioneVoto::PostazioneVoto(QObject *parent) :
    QThread(parent){
    //mainWindow = m;
    HTAssociato = "";

    attivata = false;


    myIP = this->getIPbyInterface("enp0s8");
    idPostazioneVoto = getIdPVbyMyIP(myIP);
    cout << "Postazione Voto n. " << idPostazioneVoto << endl;

    postazioneSeggio = calcolaIpSeggio(myIP); //ricavare l'IP della postazione seggio a cui la postazione voto appartiene
    ipUrna = "192.168.19.134";
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

bool PostazioneVoto::setHTAssociato(string tokenCod, string username, string password) {


    if(this->tryConnectUrna()){

        if(this->HTAssociato == ""){ //nessun token associato
            this->HTAssociato = tokenCod;
            this->usernameHTAssociato = username;
            this->passwordHTAssociato = password;

            return true;
        }
        else{
            cerr << "PV: errore di impostazione dell'HT" << endl;
            return false;
        }
    }
    return false;
}

void PostazioneVoto::resetHT()
{
    this->HTAssociato = "";
}

string PostazioneVoto::getHTAssociato() {
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
        //seleziona la scheda se l'elettore corrente assegnato alla postazione può votare per questa scheda
        vector<uint> idTipoVotanti = schedeVoto.at(i).getIdTipiVotantiConsentiti();
        for(uint t = 0; t < idTipoVotanti.size(); t++){
            if(idTipoVotante == idTipoVotanti.at(i)){

                schedeDaMostrare.push_back(schedeVoto.at(i));
                break;
            }
        }//se arriviamo alla fine di questo for, l'idTipoVotante non è presente nella scheda corrente,
        //la scheda corrente non viene selezionata tra quelle che devono essere compilate dal votante corrente


    }

    //this->setStatoPV(statiPV::votazione_in_corso);
    emit giveSchedeToView(schedeDaMostrare);
}


void PostazioneVoto::inviaVotiToUrna2(vector<SchedaCompilata> schede){
    // con i dati della schede compilate, per ogni scheda crea un file xml
    //di cui cifrare i campi contenenti i candidati votati e
    //quindi invia uno per volta i file all'urna

    bool inviati = false;
    bool erroreInvio = false;
    bool postazioneOffline = false;

    SSLClient * pv_client = new SSLClient(this);
    const char * ipUrna = this->ipUrna.c_str();
    //mi collego all'urna
    if(pv_client->connectTo(ipUrna)!=nullptr){

        uint numSchede = /*200;*/schede.size();
        cout << "PV: Schede da inviare: " << numSchede << endl;

        //1. richiedo il servizio di invio voti
        pv_client->richiestaServizioInvioSchede(numSchede);


        //per ogni scheda eseguo l'invio

        for(uint i = 0; i < numSchede/*schede.size()*/; i++){
            cout << "PV: invio pacchetto " << i+1 << endl;

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
            cout << "PV: cifro Key e IV " << endl;
            string encryptedKey = RSAencryptSecByteBlock(key,rsaPublicKeyRP);
            cout << "PV: encrypted key: " << encryptedKey << endl;
            string encryptedIV = RSAencryptSecByteBlock(iv,rsaPublicKeyRP);
            cout << "PV: encrypted IV: " << encryptedIV << endl;



            bool schedaStored = false;

            //3. invio scheda
            uint tentativiInvio = 0;
            while (!schedaStored && tentativiInvio < 10){
                tentativiInvio++;
                cout << "PV: Invio nonce, scheda  e MAC del pacchetto: " << i+1 << endl;
                cout << "PV: tentativo n. "  << tentativiInvio << endl;
                //generazione nonce

                //2. invio delle chiavi di cifratura della scheda
                pv_client->invioKC_IVC(encryptedKey, encryptedIV);

                Integer randomUint(rng,32);
                std::stringstream ss;
                ss << randomUint;
                std::string s(ss.str());
                uint nonce = atoi(s.c_str());

                cout << "PV: nonce: " << nonce << endl;

                //cifratura nonce
                string encryptedNonce = AESencryptStdString(std::to_string(nonce),key,iv);
                cout << "PV: nonce cifrato: " << encryptedNonce << endl;
                //aggiungo o sostituisco nonce nel file xml
                XMLNode *rootNode = xmlDoc.FirstChild();

                //il campo nonce, in fase di creazione scheda viene lasciato vuoto, qui inseriamo il valore
                rootNode->FirstChildElement("nonce")->SetText(encryptedNonce.c_str());


                //print file xml della scheda to string
                XMLPrinter printer;
                xmlDoc.Print( &printer );
                string schedaStr = printer.CStr();
                cout << "PV: Scheda compilata cifrata: " << schedaStr << endl;

                //generazione mac
                //dati di ingresso HMAC: scheda voto con campi candidati cifrati, chiave simmetrica e iv cifrati, nonce generato al passo precedente
                //chiave per HAMC: chiave di sessione tra pv e urna
                string datiConcatenati = schedaStr + encryptedKey + encryptedIV + std::to_string(nonce);
                cout << "PV: Dati di cui calcolare il mac: " << datiConcatenati << endl;

                string macPacchettoVoto = calcolaMAC(sessionKey_PV_Urna,datiConcatenati);

                cout << "PV: MAC del pacchetto di voto: " << macPacchettoVoto << endl;


                if(pv_client->inviaScheda_Nonce_MAC(schedaStr,std::to_string(nonce),macPacchettoVoto)){

                    //se il mac ricevuto dall'urna è univoco rispetto al db,
                    //la memorizzazione del voto andrà a buon fine
                    //settiamo schedaStored a true
                    cout << "PV: SCHEDA " <<i+1 << " ACCETTATA DALL'URNA" << endl;
                    schedaStored = true;

                }
                else{
                    cerr << "scheda non accetta dall'urna', verrà fatto un nuovo tentativo cambiando l'nonce" << endl;
                    schedaStored = false;
                }
            }//while
            if (!schedaStored){
                //una scheda non è stata inviata, nonostante i 10 tentativi
                //abortire l'invio
                erroreInvio = true;
                break;
            }
        }

        //comunicazione matricola e conferma esito positivo di ricezione schede
        cout << "PV: Comunico all'urna chi è che ha espresso il voto" << endl;
        if(!erroreInvio){
            if(pv_client->sendMatricolaAndConfirmStored(matricolaVotante)){

                inviati = true;
            }
            else{
                erroreInvio = true;
            }
        }
    }//if connect


    else{ //else connect
        //emit urnaNonRaggiungibile();

        postazioneOffline = true;
        //interrompiamo l'esecuzione della funzione, poichè non è possibile comunicare con l'urna e non sarebbe possibile inviare le schede di voto
        //le schede di voto, sono attualmente salvate nel vettore delle schedeDaInviare

    }
    delete pv_client;




    cout << "PV: tutte le schede sono state consegnate all'urna virtuale" << endl;
    //settiamo lo stato della postazione in base all'esito dell'operazione
    if(inviati){
        //tutte le schede votate sono state recapitate correttamente nell'urna
        cout << "PV: TUTTE LE SCHEDE INVIATE!!!" << endl;
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

bool PostazioneVoto::tryConnectUrna()
{
    bool urnaRaggiungibile = false;
    SSLClient * pv_client = new SSLClient(this);
    const char * ipUrna = this->ipUrna.c_str();
    if(pv_client->connectTo(ipUrna)!=nullptr){
        if(pv_client->testConnection()){
            if(attivata){
                setStatoPV(statiPV::libera);
            }
            else{
                setStatoPV(statiPV::attesa_attivazione);
            }
            urnaRaggiungibile = true;
        }
    }
    else{
        setStatoPV(statiPV::offline);
    }
    delete pv_client;

    return urnaRaggiungibile;
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

    uint idSeggio = this->idSeggio;
    pElement = xmlDoc->NewElement("idSeggio");
    pElement->SetText(idSeggio);
    pRoot->InsertEndChild(pElement);

    string descrizioneElezione = scheda.getDescrizioneElezione();
    pElement = xmlDoc->NewElement("descrizioneElezione");
    pElement->SetText(descrizioneElezione.c_str());
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
        cout << "PV: Matricola: " << matricola << endl;
        string matricolaEncrypted = AESencryptStdString(matricola,key,iv);
        cout << "PV: Matricola cifrata:" << matricolaEncrypted << endl;
        pMatr->SetText(matricolaEncrypted.c_str());
        pPreferenze->InsertEndChild(pMatr);
    }
}

string PostazioneVoto::AESencryptStdString(string plain, SecByteBlock key, SecByteBlock  iv) {
    cout << "PV: Cifratura simmetrica AES..." << endl;

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
    //cout << "PV: key: " << encoded << endl;

    // Pretty print iv
    encoded.clear();
    StringSource(iv,iv.size(),true,
                 new HexEncoder(
                     new StringSink(encoded)
                     ) // HexEncoder
                 ); // StringSource
    //cout << "PV: iv: " << encoded << endl;

    /*********************************\
    \*********************************/

    try
    {
        //cout << "PV: plain text: " << plain << endl;

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
    //cout << "PV: cipher text: " << encodedCipher << endl;

    return encodedCipher;
}

string PostazioneVoto::RSAencryptSecByteBlock(SecByteBlock valueBlock,CryptoPP::RSA::PublicKey publicKey)
{
    std::string plain = std::string(reinterpret_cast<const char*>(valueBlock.data()), valueBlock.size());
    AutoSeededRandomPool rng;

    string cipher;
    //cout << "PV: plain: " << plain << endl;

    try{
        ////////////////////////////////////////////////
        // Encryption // con la chiave pubblica di RP
        RSAES_OAEP_SHA_Encryptor rsaEncryptor( publicKey );

        StringSource( plain, true,
                      new PK_EncryptorFilter( rng, rsaEncryptor,
                                              new StringSink( cipher )
                                              ) // PK_EncryptorFilter
                      ); // StringSource

        //cout << "PV: cipher:" << cipher << endl;
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
    //cout << "PV: encoded cipher: " << encodedCipher << endl;


    // Encryption

    return encodedCipher;
}

string PostazioneVoto::calcolaIpSeggio(string ipPostazione)
{
    int byte1, byte2, byte3, byte4;
    char dot;
    istringstream s(ipPostazione);  // input stream that now contains the ip address string

    s >> byte1 >> dot >> byte2 >> dot >> byte3 >> dot >> byte4 >> dot;

    //estraiamo il valore in modulo 4 del byte meno significato dell'indirizzo ip
    int resto = byte4 % 4;

    //sottriamo a byte4 per ottenere l'indirizzo di sottorete del seggio
    int byte4Seggio = byte4-resto;

    string ipSeggio = to_string(byte1) + "." + to_string(byte2) + "." + to_string(byte3) + "." +  to_string(byte4Seggio);
    cout << "PV: ip del seggio di appartenenza: " << ipSeggio << endl;
    return ipSeggio;
}

uint PostazioneVoto::getIdPVbyMyIP(string myIP)
{
    int byte1, byte2, byte3, byte4;
    char dot;
    istringstream s(myIP);  // input stream that now contains the ip address string

    s >> byte1 >> dot >> byte2 >> dot >> byte3 >> dot >> byte4 >> dot;

    //estraiamo il valore in modulo 4 del byte meno significato dell'indirizzo ip
    //questo è l'id della Postazione Voto riferito al seggio di appartenenza
    return byte4 % 4;
}

void PostazioneVoto::inactivitySessionClose()
{
    //TODO funzione da richiamare con un thread

    //sleep 5 minuti

    //sleep conclusa

    //richiamare stato di errore della postazione di voto
}

void PostazioneVoto::validatePassKey(QString pass)
{
    //contatto l'urna per validare la password


    SSLClient * pv_client = new SSLClient(this);
    const char * ipUrna = this->ipUrna.c_str();
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
        //emit urnaNonRaggiungibile();
        this->setStatoPV(statiPV::offline);
    }

    delete pv_client;
}

void PostazioneVoto::validateOTP(QString otp)
{

    //contattare otpServer per verificare il token rispetto all'account relativo al token associato alla postazione voto
    string url = "https://147.163.26.230:8443/openotp/";
    string username = this->usernameHTAssociato;
    string password = this->passwordHTAssociato;

    char * writableURL = new char[url.size() + 1];
    std::copy(url.begin(), url.end(), writableURL);
    writableURL[url.size()] = '\0'; // don't forget the terminating 0

    char * writableUsername = new char[username.size() + 1];
    std::copy(username.begin(), username.end(), writableUsername);
    writableUsername[username.size()] = '\0'; // don't forget the terminating 0

    char * writablePassword = new char[password.size() + 1];
    std::copy(password.begin(), password.end(), writablePassword);
    writablePassword[password.size()] = '\0'; // don't forget the terminating 0

    string otpStr = otp.toStdString();
    char * writableOTP = new char[otpStr.size() + 1];
    std::copy(otpStr.begin(), otpStr.end(), writableOTP);
    writableOTP[otpStr.size()] = '\0'; // don't forget the terminating 0

    bool success= otp_login(writableURL,writableUsername,writablePassword,writableOTP);
    delete[] writableURL;
    delete[] writableUsername;
    delete[] writablePassword;
    delete[] writableOTP;
    if(success/*otp=="123456"*/){
        enablingPV();
    }
    else{
        emit wrongOTP();
    }

    // don't forget to free the string after finished using it

}

void PostazioneVoto::numberPV()
{
    emit giveNumberPV(idPostazioneVoto);
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
    cout << "PV: Dentro il thread" << endl;
    this->mutex_stdout.unlock();


    SSLClient * pv_client = new SSLClient(this);

    //il thread non vede il valore dell'ip postazioneSeggio calcolato dalla postazione di voto, lo ricalcolo
    //const char * postazioneSeggio = (this->calcolaIpSeggio(this->getIPbyInterface("enp0s8"))).c_str();
    const char * postazioneSeggio = this->postazioneSeggio.c_str();

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
    //cout << "PV: Session key: " << encodedSessionKey << endl;
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
    //cout << "PV: key encoded: " << encoded << endl;

    //cout << "PV: plain text: " << plain << endl;

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
    //cout << "PV: hmac encoded: " << macEncoded << endl;

    return macEncoded;
}

int PostazioneVoto::verifyMAC(string encodedSessionKey,string data, string macEncoded){
    //restituisce 0 in caso di verifica positiva
    //restituisce 1 in caso di verifica negativa
    string decodedKey;
    int success = 1;
    cout << "PV: Session key: " << encodedSessionKey << endl;

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
    cout << "PV: hmac decoded: " << macDecoded << endl;

    try
    {
        CryptoPP::HMAC< CryptoPP::SHA256 > hmac(key, key.size());
        const int flags = HashVerificationFilter::THROW_EXCEPTION | HashVerificationFilter::HASH_AT_END;


        StringSource(data + macDecoded, true,
                     new HashVerificationFilter(hmac, NULL, flags)
                     ); // StringSource
        success = 0;
        cout << "PV: Verified message" << endl;
    }
    catch(const CryptoPP::Exception& e)
    {
        cerr << "Caught exception :" << e.what() << endl;
    }
    return success;
}

string PostazioneVoto::getUsernameHTAssociato() const
{
    return usernameHTAssociato;
}

void PostazioneVoto::setUsernameHTAssociato(const string &value)
{
    usernameHTAssociato = value;
}

string PostazioneVoto::getPasswordHTAssociato() const
{
    return passwordHTAssociato;
}

void PostazioneVoto::setPasswordHTAssociato(const string &value)
{
    passwordHTAssociato = value;
}

uint PostazioneVoto::getIdTipoVotante() const
{
    return idTipoVotante;
}

void PostazioneVoto::setIdTipoVotante(const uint &value)
{
    idTipoVotante = value;
}

uint PostazioneVoto::getIdSeggio() const
{
    return idSeggio;
}

void PostazioneVoto::setIdSeggio(const uint &value)
{
    idSeggio = value;
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
    cout << "PV: idProcedura: " << idProcedura << endl;
    sv.setIdProceduraVoto(idProcedura);

    XMLText* textNodeIdScheda = rootNode->FirstChildElement("id")->FirstChild()->ToText();
    uint idScheda = atoi(textNodeIdScheda->Value());
    cout << "PV: idScheda: " << idScheda << endl;
    sv.setId(idScheda);

    XMLText* textNodeDescrizioneElezione= rootNode->FirstChildElement("descrizioneElezione")->FirstChild()->ToText();
    string descrizioneElezione = textNodeDescrizioneElezione->Value();
    cout << "descrizione elezione: " << descrizioneElezione << endl;
    sv.setDescrizioneElezione(descrizioneElezione);

    XMLText* textNodeNumeroPreferenze = rootNode->FirstChildElement("numeroPreferenze")->FirstChild()->ToText();
    uint numeroPreferenze = atoi(textNodeNumeroPreferenze->Value());
    cout << "PV: Numero preferenze: " << numeroPreferenze << endl;
    sv.setNumPreferenze(numeroPreferenze);

    //parsing degli idTipiVotanti
    XMLElement * tipiVotantiElement = rootNode->FirstChildElement("tipiVotanti");

    XMLElement * firstIdTipoVotantiElement = tipiVotantiElement->FirstChildElement("idTipoVotanti");
    XMLElement * lastIdTipoVotantiElement = tipiVotantiElement->LastChildElement("idTipoVotanti");

    XMLElement *idTipoVotantiElement = firstIdTipoVotantiElement;
    bool lastIdTipoVotanti = false;
    do{

        XMLText* textNodeIdTipoVotanti = idTipoVotantiElement->FirstChild()->ToText();
        uint idTipoVotanti = atoi(textNodeIdTipoVotanti->Value());
        cout << "Id tipo Votanti: " << idTipoVotanti << endl;
        sv.addIdTipiVotantiConsentiti(idTipoVotanti);


        if(idTipoVotantiElement == lastIdTipoVotantiElement){
            lastIdTipoVotanti = true;
        }
        else{
            //accediamo alla successiva lista nella scheda di voto
            idTipoVotantiElement = idTipoVotantiElement->NextSiblingElement("idTipoVotanti");
            cout << "ottengo il puntatore al successivo idTipoVotanti" << endl;
        }
    }while(!lastIdTipoVotanti);
    cout << "non ci sono altri idTipoVotanti" << endl;

    XMLElement * listeElement = rootNode->FirstChildElement("liste");

    XMLElement * firstListaElement = listeElement->FirstChildElement("lista");
    XMLElement * lastListaElement = listeElement->LastChildElement("lista");

    XMLElement *listaElement = firstListaElement;
    bool lastLista = false;
    do{

        int idLista = listaElement->IntAttribute("id");
        cout << "PV:  --- lista trovata" << endl;
        cout << "PV: id Lista: " << idLista << endl;
        string nomeLista = listaElement->Attribute("nome");
        cout << "PV: nome: " << nomeLista << endl;

        XMLElement * firstCandidatoElement  = listaElement->FirstChildElement("candidato");
        XMLElement * lastCandidatoElement = listaElement->LastChildElement("candidato");

        XMLElement * candidatoElement = firstCandidatoElement;
        //ottengo tutti i candidati della lista
        bool lastCandidato = false;
        do{
            int id = candidatoElement->IntAttribute("id");
            cout << "PV: trovato candidato, id: " << id << endl;

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

            cout << "PV: Estratti i dati del candidato id: " << id << endl;
            sv.addCandidato(matricola,nome,cognome,nomeLista,dataNascita,luogoNascita);

            //accesso al successivo candidato
            if(candidatoElement == lastCandidatoElement){
                lastCandidato = true;
            }else {
                candidatoElement = candidatoElement->NextSiblingElement("candidato");
                cout << "PV: ottengo il puntatore al successivo candidato" << endl;
            }
        }while(!lastCandidato);

        cout << "PV: non ci sono altri candidati nella lista: " << nomeLista << endl;


        if(listaElement == lastListaElement){
            lastLista = true;
        }
        else{
            //accediamo alla successiva lista nella scheda di voto
            listaElement = listaElement->NextSiblingElement("lista");
            cout << "PV: ottengo il puntatore alla successiva lista" << endl;
        }
    }while(!lastLista);
    cout << "PV: non ci sono altre liste" << endl;

    schedeVoto.push_back(sv);
}

void PostazioneVoto::clearVectorSchede()
{
    this->schedeVoto.clear();
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
    cout << "PV: publicKey decodedificata da esadecimale: " << decodedPublicKey << endl;

    StringSource ss(decodedPublicKey,true /*pumpAll*/);
    rsaPublicKeyRP.Load(ss);

    cout << "PV: la chiave publica di RP è stata salvata sulla postazione" << endl;
}


string PostazioneVoto::getMatricolaVotante() const
{
    return matricolaVotante;
}

void PostazioneVoto::setMatricolaVotante(const string &value)
{
    matricolaVotante = value;
}

string PostazioneVoto::getIPbyInterface(const char * interfaceName){
    struct ifaddrs *ifaddr, *ifa;
    int /*family,*/ s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }


    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        s=getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if((strcmp(ifa->ifa_name,interfaceName)==0)&&(ifa->ifa_addr->sa_family==AF_INET))
        {
            if (s != 0)
            {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            printf("\tInterface : <%s>\n",ifa->ifa_name );
            printf("\t  Address : <%s>\n", host);
        }
    }

    freeifaddrs(ifaddr);
    string ip_host = host;
    return ip_host;
}

string PostazioneVoto::getMyIP() const
{
    return myIP;
}

void PostazioneVoto::setMyIP(const string &value)
{
    myIP = value;
}
