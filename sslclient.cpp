#include "sslclient.h"

/*
 * sslclient.h
 *
 *  Created on: 02/apr/2017
 *      Author: giuseppe
 */



#define SERVER_PORT "4433"


SSLClient::SSLClient(PostazioneVoto *pv){
    this->pvChiamante = pv;

    this->server_sock = 0;
    /* ---------------------------------------------------------- *
     * Create the Input/Output BIO's.                             *
     * ---------------------------------------------------------- */
    this->outbio = BIO_new_fp(stdout, BIO_NOCLOSE);
    this->ssl = nullptr;
    this->server_sock = 0;
    /* ---------------------------------------------------------- *
     * Function that initialize openssl for correct work.		  *
     * ---------------------------------------------------------- */
    this->init_openssl_library();

    this->createClientContext();

    string pathCertFilePem = getConfig("clientCertPem");
    const char * certFile = pathCertFilePem.c_str();

    string pathKeyFilePem = getConfig("clientKeyPem");
    const char * keyFile = pathKeyFilePem.c_str();

    string pathChainFilePem = getConfig("chainFilePem");
    const char * chainFile = pathChainFilePem.c_str();

    this->configure_context(certFile, keyFile, chainFile);
    cout << "ClientPV:Costruttore Client: Cert and key configured" << endl;

}

SSLClient::~SSLClient(){
    /* ---------------------------------------------------------- *
     * Free the structures we don't need anymore                  *
     * -----------------------------------------------------------*/

    SSL_CTX_free(this->ctx);
    BIO_free_all(this->outbio);

    //usare con cura, cancella gli algoritmi e non funziona più nulla
    //this->cleanup_openssl();

}


SSL * SSLClient::connectTo(const char* hostIP/*hostname*/){
    pvChiamante->mutex_stdout.lock();
    cout << "ClientPV: Connecting to " << hostIP << endl;
    pvChiamante->mutex_stdout.unlock();

    this->hostIPAddress = hostIP;

    const char  port[] = SERVER_PORT;

    /* ---------------------------------------------------------- *
     * Create new SSL connection state object                     *
     * ---------------------------------------------------------- */
    this->ssl = SSL_new(this->ctx);
    //    pvChiamante->mutex_stdout.lock();
    //    cout << "ClientPV: ssl pointer: " << this->ssl << endl;
    //    pvChiamante->mutex_stdout.unlock();


    /* ---------------------------------------------------------- *
     * Make the underlying TCP socket connection                  *
     * ---------------------------------------------------------- */
    int ret = create_socket(port);


    if (ret == 0){

        pvChiamante->mutex_stdout.lock();
        BIO_printf(this->outbio,"ClientPV: Successfully create the socket for TCP connection to: %s \n",
                   this->hostIPAddress /*hostname*/);
        pvChiamante->mutex_stdout.unlock();
    }
    else {


        pvChiamante->mutex_stdout.lock();
        BIO_printf(this->outbio,"ClientPV: Unable to create the socket for TCP connection to: %s \n",
                   this->hostIPAddress /*hostname*/);
        pvChiamante->mutex_stdout.unlock();
        SSL_free(this->ssl);
        this->ssl = nullptr;
        if(close(this->server_sock) == 0){
            cerr << "ClientPV: chiusura 1 socket server" << endl;
        }
        return this->ssl;
    }

    /* ---------------------------------------------------------- *
     * Attach the SSL session to the socket descriptor            *
     * ---------------------------------------------------------- */

    if (SSL_set_fd(this->ssl, this->server_sock) != 1) {

        pvChiamante->mutex_stdout.lock();
        BIO_printf(this->outbio, "ClientPV: Error: Connection to %s failed \n", this->hostIPAddress /*hostname*/);
        pvChiamante->mutex_stdout.unlock();

        SSL_free(this->ssl);
        this->ssl = nullptr;
        if(close(this->server_sock) == 0){
            cerr << "ClientPV: chiusura 2 socket server" << endl;
        }
        return this->ssl;
    }
    else{
        pvChiamante->mutex_stdout.lock();
        BIO_printf(this->outbio, "ClientPV: Ok: Connection to %s \n", this->hostIPAddress /*hostname*/);
        pvChiamante->mutex_stdout.unlock();
    }
    /* ---------------------------------------------------------- *
     * Try to SSL-connect here, returns 1 for success             *
     * ---------------------------------------------------------- */
    ret = SSL_connect(this->ssl);
    if (ret != 1){ //SSL handshake

        pvChiamante->mutex_stdout.lock();
        BIO_printf(this->outbio, "ClientPV: Error: Could not build a SSL session to: %s\n",
                   this->hostIPAddress /*hostname*/);
        pvChiamante->mutex_stdout.unlock();

        SSL_free(this->ssl);
        this->ssl = nullptr;
        if(close(this->server_sock) == 0){
            cerr << "ClientPV: chiusura 3 socket server" << endl;
        }
        return this->ssl;
    }
    else{
        pvChiamante->mutex_stdout.lock();
        BIO_printf(this->outbio, "ClientPV: Successfully enabled SSL/TLS session to: %s\n",
                   this->hostIPAddress /*hostname*/);
        pvChiamante->mutex_stdout.unlock();
    }
    this->ShowCerts();
    this->verify_ServerCert();

    return this->ssl;
}

void SSLClient::init_openssl_library() {

    SSL_library_init();

    SSL_load_error_strings();

    ERR_load_BIO_strings();

    ERR_load_crypto_strings();

    //    /* https://www.openssl.org/docs/ssl/SSL_library_init.html */
    //    SSL_library_init();
    //    /* Cannot fail (always returns success) ??? */

    //    /* https://www.openssl.org/docs/crypto/ERR_load_crypto_strings.html */
    //    SSL_load_error_strings();
    //    /* Cannot fail ??? */

    //    ERR_load_BIO_strings();
    //    /* SSL_load_error_strings loads both libssl and libcrypto strings */
    //    //ERR_load_crypto_strings();
    //    /* Cannot fail ??? */

    //    /* OpenSSL_config may or may not be called internally, based on */
    //    /*  some #defines and internal gyrations. Explicitly call it    */
    //    /*  *IF* you need something from openssl.cfg, such as a         */
    //    /*  dynamically configured ENGINE.                              */
    //    OPENSSL_config(NULL);


    //    /* OpenSSL_config may or may not be called internally, based on */
    //    /*  some #defines and internal gyrations. Explicitly call it    */
    //    /*  *IF* you need something from openssl.cfg, such as a         */
    //    /*  dynamically configured ENGINE.                              */
    //    //OPENSSL_config(NULL);

}

void SSLClient::createClientContext(){
    const SSL_METHOD *method;
    method = TLSv1_2_client_method();

    /* ---------------------------------------------------------- *
     * Try to create a new SSL context                            *
     * ---------------------------------------------------------- */
    pvChiamante->mutex_stdout.lock();
    if ((this->ctx = SSL_CTX_new(method)) == NULL)
        BIO_printf(this->outbio, "ClientPV: Unable to create a new SSL context structure.\n");
    pvChiamante->mutex_stdout.unlock();
    /* ---------------------------------------------------------- *
     * Disabling SSLv2 and SSLv3 will leave TSLv1 for negotiation    *
     * ---------------------------------------------------------- */
    SSL_CTX_set_options(this->ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
}

int SSLClient::create_socket(const char * port) {
    /* ---------------------------------------------------------- *
     * create_socket() creates the socket & TCP-connect to server *
     * non specifica per SSL
     * ---------------------------------------------------------- */

    const char *address_printable = NULL;

    /* decomentare per usare l'hostname
     * struct hostent *host;
     */

    struct sockaddr_in dest_addr;

    unsigned int portCod = atoi(port);

    /* decommentare la sezione se si passa all'hostname invece dell'ip dell'host
    if ((host = gethostbyname(hostname)) == NULL) {
        BIO_printf(this->outbio, "ClientPV: Error: Cannot resolve hostname %s.\n", hostname);
        abort();
    }
    */

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(portCod);

    /* decommentare la sezione se si passa all'hostname invece dell'ip dell'host
    dest_addr.sin_addr.s_addr = *(long*) (host->h_addr);

    */

    // commentare la riga sotto se non si vuole usare l'ip dell'host, ma l'hostname
    dest_addr.sin_addr.s_addr = inet_addr(this->hostIPAddress);

    /* ---------------------------------------------------------- *
     * create the basic TCP socket                                *
     * ---------------------------------------------------------- */
    this->server_sock = socket(AF_INET, SOCK_STREAM, 0);
    cout << "ClientPV: Server_sock: " << this->server_sock << endl;

    /* ---------------------------------------------------------- *
     * Zeroing the rest of the struct                             *
     * ---------------------------------------------------------- */
    memset(&(dest_addr.sin_zero), '\0', 8);

    address_printable = inet_ntoa(dest_addr.sin_addr);
    //address_printable = inet_ntop(dest_addr.sin_addr);

    /* ---------------------------------------------------------- *
     * Try to make the host connect here                          *
     * ---------------------------------------------------------- */
    int res = connect(this->server_sock, (struct sockaddr *) &dest_addr, sizeof(struct sockaddr));

    pvChiamante->mutex_stdout.lock();
    if (res  == -1) {
        BIO_printf(this->outbio, "ClientPV: Error: Cannot connect to host %s [%s] on port %d.\n",
                   this->hostIPAddress /*hostname*/, address_printable, portCod);
    } else {
        BIO_printf(this->outbio, "ClientPV: Successfully connect to host %s [%s] on port %d.\n",
                   this->hostIPAddress /*hostname*/, address_printable, portCod);
        cout << "ClientPV:Descrittore socket: "<< this->server_sock << endl;
    }
    pvChiamante->mutex_stdout.unlock();

    return res;
}

void SSLClient::ShowCerts() {
    X509 *peer_cert;
    char *line;

    peer_cert = SSL_get_peer_certificate(this->ssl); /* Get certificates (if available) */

    //ERR_print_errors_fp(stderr);
    if (peer_cert != NULL) {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(peer_cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(peer_cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);
        X509_free(peer_cert);
    } else
        printf("No certificates.\n");


}

void SSLClient::configure_context(const char* CertFile, const char* KeyFile, const char * ChainFile) {
    SSL_CTX_set_ecdh_auto(this->ctx, 1);



    //---il chainfile dovrà essere ricevuto dal peer che si connette? non so se è necessario su entrambi i peer----
    SSL_CTX_load_verify_locations(this->ctx,ChainFile, NULL);
    //SSL_CTX_use_certificate_chain_file(ctx,"/home/giuseppe/myCA/intermediate/certs/ca-chain.cert.pem");
    /*The final step of configuring the context is to specify the certificate and private key to use.*/
    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(this->ctx, CertFile, SSL_FILETYPE_PEM) < 0) {
        //ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(this->ctx, KeyFile, SSL_FILETYPE_PEM) < 0) {
        //ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (!SSL_CTX_check_private_key(this->ctx)) {
        fprintf(stderr, "ClientPV: Private key does not match the public certificate\n");
        abort();
    }

}

void SSLClient::verify_ServerCert() {


    // Declare X509 structure

    X509 *error_cert = NULL;
    X509 *peer_cert = NULL;
    X509_NAME *certsubject = NULL;
    X509_STORE *store = NULL;
    X509_STORE_CTX *vrfy_ctx = NULL;
    //X509_NAME *certname = NULL;
    int ret;

    BIO *certbio = NULL;
    certbio = BIO_new(BIO_s_file());


    // Get the remote certificate into the X509 structure

    peer_cert = SSL_get_peer_certificate(this->ssl);
    pvChiamante->mutex_stdout.lock();
    if (peer_cert == NULL){
        BIO_printf(this->outbio, "ClientPV: Error: Could not get a certificate from: %s.\n",
                   this->hostIPAddress /*hostname*/);
    }
    else{
        BIO_printf(this->outbio, "ClientPV: Retrieved the server's certificate from: %s.\n",
                   this->hostIPAddress /*hostname*/);
    }
    pvChiamante->mutex_stdout.unlock();


    // extract various certificate information

    //certname = X509_NAME_new();
    //certname = X509_get_subject_name(peer_cert);

    // display the cert subject here

    //    BIO_printf(this->outbio, "ClientPV: Displaying the certificate subject data:\n");
    //    X509_NAME_print_ex(this->outbio, certname, 0, 0);
    //    BIO_printf(this->outbio, " \n");

    //Initialize the global certificate validation store object.
    if (!(store = X509_STORE_new())){
        pvChiamante->mutex_stdout.lock();
        BIO_printf(this->outbio, "ClientPV: Error creating X509_STORE_CTX object\n");
        pvChiamante->mutex_stdout.unlock();
    }
    // Create the context structure for the validation operation.
    vrfy_ctx = X509_STORE_CTX_new();

    // Load the certificate and cacert chain from file (PEM).
    /*
         ret = BIO_read_filename(certbio, certFile);
         if (!(cert = PEM_read_bio_X509(certbio, NULL, 0, NULL)))
         BIO_printf(this->outbio, "ClientPV: Error loading cert into memory\n");
         */
    char chainFile[] =
            "/home/giuseppe/myCA/intermediate/certs/ca-chain.cert.pem";

    ret = X509_STORE_load_locations(store, chainFile, NULL);


    if (ret != 1){
        pvChiamante->mutex_stdout.lock();
        BIO_printf(this->outbio, "ClientPV: Error loading CA cert or chain file\n");
        pvChiamante->mutex_stdout.unlock();
    }
    /* Initialize the ctx structure for a verification operation:
      Set the trusted cert store, the unvalidated cert, and any  *
     * potential certs that could be needed (here we set it NULL) */
    X509_STORE_CTX_init(vrfy_ctx, store, peer_cert, NULL);

    /* Check the complete cert chain can be build and validated.  *
     * Returns 1 on success, 0 on verification failures, and -1   *
     * for trouble with the ctx object (i.e. missing certificate) */
    ret = X509_verify_cert(vrfy_ctx);

    pvChiamante->mutex_stdout.lock();
    BIO_printf(this->outbio, "ClientPV: Verification return code: %d\n", ret);
    pvChiamante->mutex_stdout.unlock();

    if (ret == 0 || ret == 1){
        pvChiamante->mutex_stdout.lock();
        BIO_printf(this->outbio, "ClientPV: Verification result text: %s\n",
                   X509_verify_cert_error_string(vrfy_ctx->error));
        pvChiamante->mutex_stdout.unlock();
    }
    /* The error handling below shows how to get failure details  *
     * from the offending certificate.                            */
    if (ret == 0) {
        //get the offending certificate causing the failure
        error_cert = X509_STORE_CTX_get_current_cert(vrfy_ctx);
        certsubject = X509_NAME_new();
        certsubject = X509_get_subject_name(error_cert);

        pvChiamante->mutex_stdout.lock();
        BIO_printf(this->outbio, "ClientPV: Verification failed cert:\n");
        X509_NAME_print_ex(this->outbio, certsubject, 0, XN_FLAG_MULTILINE);
        BIO_printf(this->outbio, "ClientPV: \n");
        pvChiamante->mutex_stdout.unlock();
    }

    // Free up all structures need for verify certs
    X509_STORE_CTX_free(vrfy_ctx);
    X509_STORE_free(store);
    X509_free(peer_cert);
    BIO_free_all(certbio);

}

void SSLClient::cleanup_openssl(){
    EVP_cleanup();
}

void SSLClient::stopLocalServer(){
    //questa funzione contatta il server locale, ma non deve fare alcuna operazione se non quella
    //di sbloccare il server locale dallo stato di attesa di una nuova connessione, così da portare
    //al ricontrollo della condizione del while che se falsa, porta
    //all'interruzione del thread chiamante
    const char * port = SERVER_PORT;

    //la creazione della socket sblocca il server locale dall'accept della connessione tcp

    this->hostIPAddress="127.0.0.1";
    create_socket(port);

    // avendo impostato a true la variabile bool stopServer, non verrà inizializzata la connessione ssl
    // si passa direttamente alla chiusura delle socket
    //pvChiamante->mutex_stdout.lock();
    pvChiamante->mutex_stdout.lock();
    cout << "ClientPV: niente da fare... chiudo la socket per il server" << endl;
    pvChiamante->mutex_stdout.unlock();
    //pvChiamante->mutex_stdout.unlock();

    if(close(this->server_sock) != 0)
    {
        cerr << "ClientPV: errore chiusura socket server" << endl;
    }

}

void SSLClient::updateStatoPVtoSeggio(unsigned int idPV, unsigned int statoPV){
    //comunica al seggio come è cambiato lo stato della postazione di voto.
    pvChiamante->mutex_stdout.lock();
    cout << "ClientPV: Try to update..." << endl;
    pvChiamante->mutex_stdout.unlock();

    stringstream ss;
    ss << idPV;
    string str= ss.str();
    const char * charArray_idPV = str.c_str();
    pvChiamante->mutex_stdout.lock();
    cout << "ClientPV: idPV to update: " << charArray_idPV << endl;
    pvChiamante->mutex_stdout.unlock();

    //cout << strlen(charArray_idPV) << endl;
    SSL_write(ssl, charArray_idPV, strlen(charArray_idPV));

    stringstream ss1;
    ss1 << statoPV;
    const char *  charArray_statoPV = ss1.str().c_str();
    pvChiamante->mutex_stdout.lock();
    cout << "ClientPV: statoPV to update: " << charArray_statoPV << endl;
    pvChiamante->mutex_stdout.unlock();

    SSL_write(ssl, charArray_statoPV, strlen(charArray_statoPV));

    pvChiamante->mutex_stdout.lock();
    BIO_printf(this->outbio, "ClientPV: Finished SSL/TLS connection with server: %s.\n",
               this->hostIPAddress);
    pvChiamante->mutex_stdout.unlock();
    int ret = SSL_shutdown(this->ssl);
    if (ret == 0){
        SSL_shutdown(this->ssl);
    }

    SSL_free(this->ssl);

    if(close(this->server_sock) != 0)
        cerr << "errore chiusura della socket creata per il server" << endl;

}

bool SSLClient::attivaPostazioneVoto(string sessionKey)
{
    //nel caso in cui un tentativo di attavazione non si sia completato con successo
    //il vettore delle schede di voto potrebbe aver memorizzato alcune schede
    //per sicurezza bisogna svuotarlo prima di procedere con un nuovo tentativo di attivazione
    pvChiamante->clearVectorSchede();

    bool attivata = false;
    //richiesta servizio
    int serviceCod = serviziUrna::attivazionePV;
    stringstream ssCod;
    ssCod << serviceCod;
    string strCod = ssCod.str();
    const char * charCod = strCod.c_str();
    pvChiamante->mutex_stdout.lock();
    cout << "ClientPV: richiedo il servizio: " << charCod << endl;
    pvChiamante->mutex_stdout.unlock();
    SSL_write(ssl,charCod,strlen(charCod));

    //invio mio ip
    sendString_SSL(ssl,/*my_ip*/pvChiamante->getMyIP());

    //ricevo idProceduraVoto
    uint idProcedura;
    char cod_idProcedura[128];
    memset(cod_idProcedura, '\0', sizeof(cod_idProcedura));
    int bytes = SSL_read(ssl, cod_idProcedura, sizeof(cod_idProcedura));
    if (bytes > 0) {
        cod_idProcedura[bytes] = 0;
        idProcedura = atoi(cod_idProcedura);
        //pvChiamante->setIdProceduraVoto(idProcedura);
    }
    else{
        cerr << "ClientPV: non sono riuscito a ricevere l'idProcedura" << endl;
    }

    //SE L'ID PROCEDURA RICEVUTO È 0, NON C'È UNA PROCEDURA IN CORSO, ABORTISCO L'ATTIVAZIONE
    if(idProcedura == 0){
        return attivata; //valore corrente "false"

    }
    else{
        pvChiamante->setIdProceduraVoto(idProcedura);
    }


    cout << "ClientPV: La procedura in corso ha id: " << idProcedura << endl;
    string idProceduraMAC = pvChiamante->calcolaMAC(sessionKey, to_string(idProcedura));

    //invio MAC all'URNA

    const char * charIdProceduraMAC = idProceduraMAC.c_str();
    //uvChiamante->mutex_stdout.lock();
    cout << "ClientPV: Invio il MAC all'Urna: " << charIdProceduraMAC << endl;
    //uvChiamante->mutex_stdout.unlock();
    SSL_write(ssl,charIdProceduraMAC,strlen(charIdProceduraMAC));


    //ricevi esito verifica della chiave di sessione
    // 0 -> success
    // 1 -> error

    char buffer[16];
    memset(buffer, '\0', sizeof(buffer));
    bytes = SSL_read(ssl,buffer,sizeof(buffer));
    if(bytes > 0){
        buffer[bytes] = 0;
        int result = atoi(buffer);
        cout << "ClientPV: esito verifica MAC: " << result << endl;
        if (result == 0){
            attivata = true;
        }

    }

    //se l'attivazione ha avuto successo ricevo le schede dall'urna, la chiave pubblica di RP e l'id del Seggio a cui appartiene questa postazione
    if(attivata){
        //ricevo numero schede da ricevere
        string str;
        receiveString_SSL(ssl,str);
        uint numSchede = atoi(str.c_str());
        cout << "ClientPV: Numero schede da ricevere: " << numSchede << endl;

        //ricevo schede
        for(uint i = 0; i< numSchede; i++){

            string scheda;
            receiveString_SSL(ssl, scheda);
            if(scheda ==""){
                attivata = false; // ricezione schede non riuscita
                break;
            }
            cout << scheda << endl;

            //ricevo MAC i-esima scheda di voto
            string encodedMAC;
            receiveString_SSL(ssl,encodedMAC);
            int verified = pvChiamante->verifyMAC(sessionKey,scheda,encodedMAC);
            if(verified == 0){
                pvChiamante->addScheda(scheda);
                cout << "ClientPV: scheda " << i+1 << " ricevuta" << endl;
            }
            else{
                attivata = false; // ricezione schede non riuscita
                break;
            }
        }

        if(attivata){
            //ricevo chiave pubblica RP
            string publicKeyRP;
            receiveString_SSL(ssl,publicKeyRP);
            cout << "ClientPV: publicKey RP: " << publicKeyRP << endl;

            //ricevo mac chiave pubblica RP  e verifico
            string encodedMAC;
            receiveString_SSL(ssl,encodedMAC);
            int verified = pvChiamante->verifyMAC(sessionKey,publicKeyRP,encodedMAC);
            if(verified == 0){//MAC chiave pubblica RP verifcato
                pvChiamante->setRSAPublicKeyRP(publicKeyRP);
            }
            else{
                attivata = false; //ricezione chiave pubblica RP non riuscita
            }
        }
        if (attivata){
            //ricevo id Seggio di appartenenza
            string seggio;
            receiveString_SSL(ssl, seggio);

            string encodedMAC;
            receiveString_SSL(ssl,encodedMAC);
            int verified = pvChiamante->verifyMAC(sessionKey,seggio,encodedMAC);
            if(verified == 0){//MAC chiave pubblica RP verifcato
                uint idSeggio = atoi(seggio.c_str());
                pvChiamante->setIdSeggio(idSeggio);
            }
            else{
                attivata = false; //ricezione chiave pubblica RP non riuscita
            }



        }


    }
    return attivata;
}

void SSLClient::richiestaServizioInvioSchede(uint numSchede){
    //richiesta servizio
    int serviceCod = serviziUrna::invioSchedeCompilate;
    stringstream ssCod;
    ssCod << serviceCod;
    string strCod = ssCod.str();
    const char * charCod = strCod.c_str();
    pvChiamante->mutex_stdout.lock();
    cout << "ClientPV: richiedo il servizio: " << charCod << endl;
    pvChiamante->mutex_stdout.unlock();
    SSL_write(ssl,charCod,strlen(charCod));

    //invio mio ip
    sendString_SSL(ssl,/*my_ip*/pvChiamante->getMyIP());



    //invio del numero di schede che si vuole inviare
    sendString_SSL(ssl,to_string(numSchede));
}

void SSLClient::invioKC_IVC(string encryptedKey, string encryptedIV){

    cout << "ClientPV: Invio chiave cifrata e iv cifrato..." << endl;
    sendString_SSL(ssl,encryptedKey);
    sendString_SSL(ssl,encryptedIV);
}

bool SSLClient::inviaScheda_Nonce_MAC(string schedaStr,string nonceAsString,string macPacchettoVoto){
    //invio scheda cifata
    sendString_SSL(ssl,schedaStr);

    //invio nonce sotto forma di stringa
    sendString_SSL(ssl,nonceAsString);


    //invio mac generato dal pacchetto di voto
    sendString_SSL(ssl,macPacchettoVoto);


    //ricevi esito operazione di ricezione scheda dall'urna
    uint esito = 1;
    string s;
    receiveString_SSL(ssl,s);
    if(s!=""){
        esito = atoi(s.c_str());
    }

    if(esito == 0){

        cout << "ClientPV: Scheda inviata correttamente, id: " << macPacchettoVoto << endl;
        return true;
    }
    else{
        cerr << "Reinvio scheda necessario" << endl;
        return false;
    }


}



bool SSLClient::sendMatricolaAndConfirmStored(string matricola){
    //invia matricola
    sendString_SSL(ssl,matricola);

    //ricevo conferma che i pacchetti stanno per essere memorizzati
    string ack;
    receiveString_SSL(ssl,ack);

    if(ack == "ACK"){
        //invio ack di risposta
        sendString_SSL(ssl,"ACK");
    }

    //esito operazione di invio voti
    string s;
    int success = 0;
    if(receiveString_SSL(ssl, s)!=0){

        success = atoi(s.c_str());


        if(success == 0){
            return true;
        }
        else{
            return false;
        }
    }
    else {
        return false;
    }

}

//bool SSLClient::setVoted(uint matricola)
//{
//    //richiesta servizio
//    int serviceCod = serviziUrna::setMatricolaVoted;
//    stringstream ssCod;
//    ssCod << serviceCod;
//    string strCod = ssCod.str();
//    const char * charCod = strCod.c_str();
//    pvChiamante->mutex_stdout.lock();
//    cout << "ClientPV: richiedo il servizio: " << charCod << endl;
//    pvChiamante->mutex_stdout.unlock();
//    SSL_write(ssl,charCod,strlen(charCod));

//    bool settedVoted = false;

//    //invia matricola
//    sendString_SSL(ssl,to_string(matricola));

//    //ricevi esito operazione
//    string s;
//    receiveString_SSL(ssl,s);
//    int success = atoi(s.c_str());
//    if(success==0){
//        settedVoted = true;
//    }

//    return settedVoted;
//}



bool SSLClient::testConnection(){
    //richiesta servizio
    int serviceCod = serviziUrna::checkConnection;
    stringstream ssCod;
    ssCod << serviceCod;
    string strCod = ssCod.str();
    const char * charCod = strCod.c_str();
    pvChiamante->mutex_stdout.lock();
    cout << "ClientPV: richiedo il servizio: " << charCod << endl;
    pvChiamante->mutex_stdout.unlock();
    SSL_write(ssl,charCod,strlen(charCod));


    sendString_SSL(ssl,pvChiamante->getMyIP() );
    string connectionOK;
    if(receiveString_SSL(ssl,connectionOK)!=0){
        if (connectionOK == "ok"){
            return true;
        }
        else return false;
    }

    return false;
    //nothing to do
}

int SSLClient::receiveString_SSL(SSL* ssl, string &s){

    char dim_string[16];
    memset(dim_string, '\0', sizeof(dim_string));
    int bytes = SSL_read(ssl, dim_string, sizeof(dim_string));
    if (bytes > 0) {
        dim_string[bytes] = 0;
        //lunghezza fileScheda da ricevere
        uint length = atoi(dim_string);
        char buffer[length + 1];
        memset(buffer, '\0', sizeof(buffer));
        bytes = SSL_read(ssl, buffer, sizeof(buffer));
        if (bytes > 0) {
            buffer[bytes] = 0;
            s = buffer;
        }
    }
    return bytes; //bytes read for the string received
}

void SSLClient::sendString_SSL(SSL* ssl, string s) {
    int length = strlen(s.c_str());
    string length_str = std::to_string(length);
    const char *num_bytes = length_str.c_str();
    SSL_write(ssl, num_bytes, strlen(num_bytes));
    SSL_write(ssl, s.c_str(), length);
}


