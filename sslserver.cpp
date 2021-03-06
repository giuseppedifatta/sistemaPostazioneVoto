#include "sslserver.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <mutex>
#include <vector>
#include <thread>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>



# define HOST_NAME "localhost"

#define PORT "4433"

using namespace std;

SSLServer::SSLServer(PostazioneVoto *pv){
    this->setStopServer(false);
    this->pvChiamante = pv;

    this->init_openssl_library();
    this->createServerContext();

//    const char *certFile = "/home/giuseppe/myCA/intermediate/certs/client.cert.pem";
//    const char *keyFile = "/home/giuseppe/myCA/intermediate/private/client.key.pem";
//    const char *chainFile = "/home/giuseppe/myCA/intermediate/certs/ca-chain.cert.pem";
    string s1 = getConfig("clientCertPem");
    const char *certFile = s1.c_str();

    string s2 = getConfig("clientKeyPem");
    const char *keyFile = s2.c_str();

    string s3 = getConfig("chainFilePem");
    const char *chainFile = s3.c_str();

    configure_context(certFile, keyFile, chainFile);

    pvChiamante->mutex_stdout.lock();
    cout << "ServerPV: Cert and key configured" << endl;
    pvChiamante->mutex_stdout.unlock();


    this->listen_sock = this->openListener(atoi(PORT));
    this->outbio = BIO_new_fp(stdout, BIO_NOCLOSE);

    setStopServer(false);

}

SSLServer::~SSLServer(){
    //nel distruttore


    BIO_free_all(this->outbio);
    SSL_CTX_free(this->ctx);

    this->cleanup_openssl();

    close(listen_sock);

}

void SSLServer::ascoltaSeggio(){

    //inizializza una socket per il client
    struct sockaddr_in client_addr;
    uint len = sizeof(client_addr);

    pvChiamante->mutex_stdout.lock();
    cout << "ServerPV: in ascolto sulla porta " << PORT << ", attesa connessione da un client PV...\n";
    pvChiamante->mutex_stdout.unlock();

    // accept restituisce un valore negativo in caso di insuccesso
    int client_sock = accept(this->listen_sock, (struct sockaddr*) &client_addr, &len);

    if (client_sock < 0) {
        perror("Unable to accept");
        exit(EXIT_FAILURE);
    }
    else{
        unsigned int port_assigned = ntohs(client_addr.sin_port);
        pvChiamante->mutex_stdout.lock();
        cout << "ServerPV: Un client ha iniziato la connessione su una socket con fd:" << client_sock << endl;
        cout << "ServerPV: Client's Port assegnata: "<< port_assigned << endl;
        pvChiamante->mutex_stdout.unlock();

    }

    if(!(this->stopServer)){
        //sezione 1
        //se non è stata settata l'interruzione del server, lancia il thread per servire la richiesta
        //commentare la sezione 2 per usare questa sezione
        std::thread t (&SSLServer::Servlet, this, client_sock);
        t.detach();
        pvChiamante->mutex_stdout.lock();
        cout << "ServerPV: sub_thread started..." << endl;
        pvChiamante->mutex_stdout.unlock();


        //-----sezione sperimentale-----
        //sezione 2
        //proviamo a non lanciare il thread e fare le operazioni sequenzialmente nel thread server
        //principale, senza generare un thread figlio che svolga il compito al suo posto
        //commentare la sezione 1 per usare questa sezione
        //        pvChiamante->mutex_stdout.lock();
        //        cout << "ServerPV: eseguo il servizio richiesto nel thread server principale" << endl;
        //        pvChiamante->mutex_stdout.unlock();

        //        Servlet(client_sock);

    }
    else{
        //termina l'ascolto


        int ret = close(client_sock);


        cout << "ServerPV: interruzione del server in corso..." << endl;
        if(ret ==0){
            pvChiamante->mutex_stdout.lock();
            cout << "ServerPV: successo chiusura socket per il client" << endl;
            pvChiamante->mutex_stdout.unlock();
        }
        ret = close(this->listen_sock);
        if(ret ==0){
            pvChiamante->mutex_stdout.lock();
            cout << "ServerPV: successo chiusura socket del listener" << endl;
            pvChiamante->mutex_stdout.unlock();
        }

        return;
    }

}

void SSLServer::Servlet(int client_sock) {/* Serve the connection -- threadable */
    pvChiamante->mutex_stdout.lock();
    cout << "ServerPV: Servlet: inizio servlet" << endl;
    pvChiamante->mutex_stdout.unlock();

    SSL * ssl = SSL_new(ctx);
    //collega sessione ssl alla socket assegnata al client
    SSL_set_fd(ssl, client_sock);


    if (SSL_accept(ssl) <= 0) {/* do SSL-protocol handshake */
        pvChiamante->mutex_stdout.lock();
        cout << "ServerPV: error in handshake" << endl;
        pvChiamante->mutex_stdout.unlock();
        ERR_print_errors_fp(stderr);

    }
    else {
        pvChiamante->mutex_stdout.lock();
        cout << "ServerPV: handshake ok!" << endl;
        pvChiamante->mutex_stdout.unlock();
        this->ShowCerts(ssl);
        this->verify_ClientCert(ssl);

        pvChiamante->mutex_stdout.lock();
        cout << "ServerPV: ricevo l'identificativo del servizio richiesto..." << endl;
        pvChiamante->mutex_stdout.unlock();

        //ricezione codice del servizio richiesto
        int servizio;
        char cod_servizio[128];
        memset(cod_servizio, '\0',sizeof(cod_servizio));
        int bytes = SSL_read(ssl,cod_servizio,sizeof(cod_servizio));
        if(bytes>0){
            cod_servizio[bytes]= 0;
            servizio = atoi(cod_servizio);
            servizi s;
            switch(servizio){
            case 0:
                s = servizi::setAssociation;
                this->service(ssl, s);
                break;
            case 1:
                s = servizi::pullPVState;
                this->service(ssl, s);
                break;
            case 2:
                s = servizi::removeAssociation;
                this->service(ssl, s);
                break;
            case 3:
                s = servizi::freePV;
                this->service(ssl, s);
                break;
            default:
                cerr << "codice servizio non valido" << endl;
            }
        }
        else{
            cerr << "ServerPV: errore lettura del codice di servizio" <<endl;
        }
    }
    int sd;
    sd = SSL_get_fd(ssl); /* get socket connection */

    /* close connection */
    if(close(sd)!=0){
        cerr << "ServerPV: Errore di chiusura socket sub_thread " << endl;
    }
    SSL_free(ssl);

    pvChiamante->mutex_stdout.lock();
    cout << "ServerPV: fine servlet" << endl;
    pvChiamante->mutex_stdout.unlock();
    //close(client_sock);


}

void SSLServer::service(SSL * ssl, servizi servizio) {
    pvChiamante->mutex_stdout.lock();
    cout << "ServerPV: service started: " << servizio << endl;
    pvChiamante->mutex_stdout.unlock();


    switch (servizio) {

    case servizi::setAssociation:{


        bool erroreRicezioneDati = false;
        string snHT;
        unsigned int idTipoVotante;
        string usernameHT;
        string passwordHT;

        //ricevi ht da settare
        if(receiveString_SSL(ssl,snHT)==0){
            erroreRicezioneDati = true;
        }
        pvChiamante->mutex_stdout.lock();
        cout << "ServerPV: ht da settare: " << snHT << endl;
        pvChiamante->mutex_stdout.unlock();

        //ricevi idTipoVotante
        string strTipo;
        if(receiveString_SSL(ssl,strTipo)!=0){
            cout << "Tipo votante: " << strTipo << endl;
            idTipoVotante = atoi(strTipo.c_str());
        }
        else{
            erroreRicezioneDati = true;
        }

        //ricevi matricola votante
        string matricola;
        if(receiveString_SSL(ssl,matricola)==0){
            erroreRicezioneDati = true;
        }
        cout << "Matricola: "<< matricola << endl;
        //ricezione usernameHT
        if(receiveString_SSL(ssl,usernameHT)==0){
            erroreRicezioneDati = true;
        }
        cout << "usernameHT: " << usernameHT << endl;

        //ricezione passwordHT
        if(receiveString_SSL(ssl,passwordHT)==0){
            erroreRicezioneDati = true;
        }
        cout << "passwordHT: " <<  passwordHT << endl;

        int success = 1;
        if(!erroreRicezioneDati && pvChiamante->setHTAssociato(snHT,usernameHT,passwordHT)){ //restituisce true se l'esito dell'operazione è positivo
            success = 0;
            cout << "NO PROBLEM"<< endl;
            //settiamo le altre informazioni relative al votante
            pvChiamante->setMatricolaVotante(matricola);
            pvChiamante->setIdTipoVotante(idTipoVotante);
            pvChiamante->setStatoPV(pvChiamante->statiPV::attesa_abilitazione);
        }

        //inviamo al seggio il valore relativo al successo o all'insuccesso dell'operazione
        pvChiamante->mutex_stdout.lock();
        cout << "ServerPV: setAssociation: return value to seggio: " << success << endl;
        pvChiamante->mutex_stdout.unlock();

        //inviamo al seggio un codice relativo all'esito dell'operazione
        sendString_SSL(ssl,to_string(success));

        break;
    }
    case servizi::pullPVState:{

        stringstream ss1;
        ss1 << pvChiamante->getStatoPV();
        const char *  charArray_statoPV = ss1.str().c_str();

        pvChiamante->mutex_stdout.lock();
        cout << "ServerPV: statoPV richiesto dal seggio: " << charArray_statoPV << endl;
        pvChiamante->mutex_stdout.unlock();

        SSL_write(ssl, charArray_statoPV, strlen(charArray_statoPV));


        uint stato = atoi(charArray_statoPV);
        if(stato == pvChiamante->statiPV::offline){
            pvChiamante->tryConnectUrna();
        }
        break;
    }
    case servizi::removeAssociation:{
        int success = -1;
        if( (pvChiamante->getStatoPV() == pvChiamante->statiPV::attesa_abilitazione)
                || (pvChiamante->getStatoPV() == pvChiamante->statiPV::offline)
                || (pvChiamante->getStatoPV() == pvChiamante->statiPV::errore)){
            //imposta stato postazione a libera

            pvChiamante->resetHT();
            pvChiamante->setStatoPV(pvChiamante->statiPV::libera);
            success = 0;
            pvChiamante->mutex_stdout.lock();
            cout << "ServerPV: associazione rimossa!! "<< endl;
            pvChiamante->mutex_stdout.unlock();

        }

        //inviamo al seggio il valore relativo al successo o all'insuccesso dell'operazione
        stringstream ss;
        ss << success;
        string str = ss.str();
        pvChiamante->mutex_stdout.lock();
        cout << "ServerPV: removeAssociation: return value to seggio: " << success << endl;
        pvChiamante->mutex_stdout.unlock();

        //inviamo al seggio un codice relativo all'esito dell'operazione
        const char * successValue = str.c_str();
        SSL_write(ssl,successValue,strlen(successValue));
        break;
    }
    case servizi::freePV:{
        int success = -1;
        if( pvChiamante->getStatoPV() == pvChiamante->statiPV::votazione_completata ){
            //imposta stato postazione a libera

            pvChiamante->resetHT();
            pvChiamante->setStatoPV(pvChiamante->statiPV::libera);
            success = 0;
            pvChiamante->mutex_stdout.lock();
            cout << "ServerPV: postazione liberata! "<< endl;
            pvChiamante->mutex_stdout.unlock();

        }

        //inviamo al seggio il valore relativo al successo o all'insuccesso dell'operazione

        string str = to_string(success);
        pvChiamante->mutex_stdout.lock();
        cout << "ServerPV: freePV: return value to seggio: " << success << endl;
        pvChiamante->mutex_stdout.unlock();

        //inviamo al seggio un codice relativo all'esito dell'operazione
        const char * successValue = str.c_str();
        SSL_write(ssl,successValue,strlen(successValue));
        break;

    }
    }

    return;

}

int SSLServer::openListener(int s_port) {

    // non è specifico per openssl, crea una socket in ascolto su una porta passata come argomento
    int  r;

    struct sockaddr_in sa_serv;
    this->listen_sock = socket(PF_INET, SOCK_STREAM, 0);

    //allow reuse of port without dealy for TIME_WAIT
    int iSetOption = 1;
    setsockopt(this->listen_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption,
               sizeof(iSetOption));

    if (this->listen_sock <= 0) {
        perror("Unable to create socket");
        abort();
    }

    memset(&sa_serv, 0, sizeof(sa_serv));
    sa_serv.sin_family = AF_INET;
    sa_serv.sin_addr.s_addr = INADDR_ANY;
    sa_serv.sin_port = htons(s_port); /* Server Port number */
    pvChiamante->mutex_stdout.lock();
    cout << "ServerPV: Server's Port: "<< ntohs(sa_serv.sin_port)<<endl;
    pvChiamante->mutex_stdout.unlock();
    r = bind(this->listen_sock, (struct sockaddr*) &sa_serv, sizeof(sa_serv));
    if (r < 0) {
        perror("Unable to bind");
        exit(EXIT_FAILURE);
    }

    // Receive a TCP connection.
    r = listen(this->listen_sock, 10);

    if (r < 0) {
        perror("Unable to listen");
        exit(EXIT_FAILURE);
    }
    return this->listen_sock;
}


void SSLServer::createServerContext() {
    const SSL_METHOD *method;
    method = TLSv1_2_server_method();

    this->ctx = SSL_CTX_new(method);
    if (!this->ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    //https://www.openssl.org/docs/man1.1.0/ssl/SSL_CTX_set_options.html
    const long flags = SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION;
    /*long old_opts = */SSL_CTX_set_options(this->ctx, flags);
    //    pvChiamante->mutex_stdout.lock();
    //    cout << "ServerPV: bitmask options: " << old_opts << endl;
    //    pvChiamante->mutex_stdout.unlock();
    //return ctx;
}

void SSLServer::configure_context(const char* CertFile, const char* KeyFile, const char* ChainFile) {
    SSL_CTX_set_ecdh_auto(this->ctx, 1);

    SSL_CTX_load_verify_locations(this->ctx, ChainFile, ChainFile);
    //SSL_CTX_use_certificate_chain_file(ctx,"/home/giuseppe/myCA/intermediate/certs/ca-chain.cert.pem");

    /*The final step of configuring the context is to specify the certificate and private key to use.*/
    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(this->ctx, CertFile, SSL_FILETYPE_PEM) < 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(this->ctx, KeyFile, SSL_FILETYPE_PEM) < 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);

    }

    //SSL_CTX_set_default_passwd_cb(ctx,"password"); // cercare funzionamento con reference

    if (!SSL_CTX_check_private_key(this->ctx)) {
        fprintf(stderr, "ServerPV: Private key does not match the public certificate\n");
        abort();
    }
    //substitute NULL with the name of the specific verify_callback
    SSL_CTX_set_verify(this->ctx, SSL_VERIFY_PEER, NULL);

}

void SSLServer::init_openssl_library() {
    /* https://www.openssl.org/docs/ssl/SSL_library_init.html */
    SSL_library_init();
    /* Cannot fail (always returns success) ??? */

    /* https://www.openssl.org/docs/crypto/ERR_load_crypto_strings.html */
    SSL_load_error_strings();
    /* Cannot fail ??? */

    /* SSL_load_error_strings loads both libssl and libcrypto strings */
    ERR_load_crypto_strings();
    /* Cannot fail ??? */

    /* OpenSSL_config may or may not be called internally, based on */
    /*  some #defines and internal gyrations. Explicitly call it    */
    /*  *IF* you need something from openssl.cfg, such as a         */
    /*  dynamically configured ENGINE.                              */
    OPENSSL_config(NULL);

}

void SSLServer::print_cn_name(const char* label, X509_NAME* const name) {
    int idx = -1, success = 0;
    unsigned char *utf8 = NULL;

    do {
        if (!name)
            break; /* failed */

        idx = X509_NAME_get_index_by_NID(name, NID_commonName, -1);
        if (!(idx > -1))
            break; /* failed */

        X509_NAME_ENTRY* entry = X509_NAME_get_entry(name, idx);
        if (!entry)
            break; /* failed */

        ASN1_STRING* data = X509_NAME_ENTRY_get_data(entry);
        if (!data)
            break; /* failed */

        int length = ASN1_STRING_to_UTF8(&utf8, data);
        if (!utf8 || !(length > 0))
            break; /* failed */

        pvChiamante->mutex_stdout.lock();
        cout << "ServerPV: " << label << ": " << utf8 << endl;
        pvChiamante->mutex_stdout.unlock();
        success = 1;

    } while (0);

    if (utf8)
        OPENSSL_free(utf8);

    if (!success){
        pvChiamante->mutex_stdout.lock();
        cout << "ServerPV: " << label << ": <not available>" << endl;
        pvChiamante->mutex_stdout.unlock();
    }

}

void SSLServer::print_san_name(const char* label, X509* const cert) {
    int success = 0;
    GENERAL_NAMES* names = NULL;
    unsigned char* utf8 = NULL;

    do {
        if (!cert)
            break; // failed

        names = (GENERAL_NAMES*) X509_get_ext_d2i(cert, NID_subject_alt_name, 0,
                                                  0);
        if (!names)
            break;

        int i = 0, count = sk_GENERAL_NAME_num(names);
        if (!count)
            break; // failed

        for (i = 0; i < count; ++i) {
            GENERAL_NAME* entry = sk_GENERAL_NAME_value(names, i);
            if (!entry)
                continue;

            if (GEN_DNS == entry->type) {
                int len1 = 0, len2 = -1;
                //ASN1_STRING_to_UTF8 restiruisce la lunghezza del buffer di out o un valore negativo
                len1 = ASN1_STRING_to_UTF8(&utf8, entry->d.dNSName);
                if (utf8) {
                    len2 = (int) strlen((const char*) utf8);
                }

                if (len1 != len2) {
                    cerr
                            << "  Strlen and ASN1_STRING size do not match (embedded null?): "
                            << len2 << " vs " << len1 << endl;
                }

                // If there's a problem with string lengths, then
                // we skip the candidate and move on to the next.
                // Another policy would be to fails since it probably
                // indicates the client is under attack.
                if (utf8 && len1 && len2 && (len1 == len2)) {
                    pvChiamante->mutex_stdout.lock();
                    cout << "ServerPV: " << label << ": " << utf8 << endl;
                    pvChiamante->mutex_stdout.unlock();
                    success = 1;
                }

                if (utf8) {
                    OPENSSL_free(utf8), utf8 = NULL;
                }
            } else {
                cerr << "  Unknown GENERAL_NAME type: " << entry->type << endl;
            }
        }

    } while (0);

    if (names)
        GENERAL_NAMES_free(names);

    if (utf8)
        OPENSSL_free(utf8);

    if (!success){
        pvChiamante->mutex_stdout.lock();
        cout << "ServerPV: " << label << ": <not available>\n" << endl;
        pvChiamante->mutex_stdout.unlock();
    }
}

int SSLServer::verify_callback(int preverify, X509_STORE_CTX* x509_ctx) {

    /*cout << "ServerPV: preverify value: " << preverify <<endl;*/
    int depth = X509_STORE_CTX_get_error_depth(x509_ctx);
    int err = X509_STORE_CTX_get_error(x509_ctx);

    X509* cert = X509_STORE_CTX_get_current_cert(x509_ctx);
    X509_NAME* iname = cert ? X509_get_issuer_name(cert) : NULL;
    X509_NAME* sname = cert ? X509_get_subject_name(cert) : NULL;

    pvChiamante->mutex_stdout.lock();
    cout << "ServerPV: verify_callback (depth=" << depth << ")(preverify=" << preverify
         << ")" << endl;
    pvChiamante->mutex_stdout.unlock();
    /* Issuer is the authority we trust that warrants nothing useful */
    print_cn_name("Issuer (cn)", iname);

    /* Subject is who the certificate is issued to by the authority  */
    print_cn_name("Subject (cn)", sname);

    if (depth == 0) {
        /* If depth is 0, its the server's certificate. Print the SANs */
        print_san_name("Subject (san)", cert);
    }


    pvChiamante->mutex_stdout.lock();
    if (preverify == 0) {
        if (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY){

            cout << "ServerPV: Error = X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY\n";
        }
        else if (err == X509_V_ERR_CERT_UNTRUSTED){

            cout << "ServerPV: Error = X509_V_ERR_CERT_UNTRUSTED\n";
        }
        else if (err == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN){

            cout << "ServerPV: Error = X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN\n";}
        else if (err == X509_V_ERR_CERT_NOT_YET_VALID) {

            cout << "ServerPV: Error = X509_V_ERR_CERT_NOT_YET_VALID\n";
        }
        else if (err == X509_V_ERR_CERT_HAS_EXPIRED){

            cout << "ServerPV: Error = X509_V_ERR_CERT_HAS_EXPIRED\n";
        }
        else if (err == X509_V_OK){

            cout << "ServerPV: Error = X509_V_OK\n";
        }
        else{

            cout << "ServerPV: Error = " << err << "\n";
        }
    }
    pvChiamante->mutex_stdout.unlock();

    return 1;
}

void SSLServer::print_error_string(unsigned long err, const char* const label) {
    const char* const str = ERR_reason_error_string(err);
    if (str)
        fprintf(stderr, "%s\n", str);
    else
        fprintf(stderr, "%s failed: %lu (0x%lx)\n", label, err, err);
}

void SSLServer::cleanup_openssl() {
    EVP_cleanup();
}

void SSLServer::ShowCerts(SSL * ssl) {

    X509 *cert = NULL;
    char *line = NULL;
    cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */


    ERR_print_errors_fp(stderr);
    if (cert != NULL) {
        pvChiamante->mutex_stdout.lock();
        BIO_printf(this->outbio, "ServerPV::Client certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        BIO_printf(this->outbio, "ServerPV::Subject: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        BIO_printf(this->outbio, "ServerPV::Issuer: %s\n", line);
        free(line);
        pvChiamante->mutex_stdout.unlock();
    } else{
        pvChiamante->mutex_stdout.lock();
        BIO_printf(this->outbio, "ServerPV::No certificates.\n");
        pvChiamante->mutex_stdout.unlock();
    }
    X509_free(cert);

}

void SSLServer::verify_ClientCert(SSL *ssl) {

    /* ---------------------------------------------------------- *
     * Declare X509 structure                                     *
     * ---------------------------------------------------------- */
    X509 *error_cert = NULL;
    X509 *cert = NULL;
    X509_NAME *certsubject = NULL;
    X509_STORE *store = NULL;
    X509_STORE_CTX *vrfy_ctx = NULL;
    //BIO *certbio = NULL;
    //X509_NAME *certname = NULL;

    //certbio = BIO_new(BIO_s_file());

    /* ---------------------------------------------------------- *
     * Get the remote certificate into the X509 structure         *
     * ---------------------------------------------------------- */
    cert = SSL_get_peer_certificate(ssl);
    pvChiamante->mutex_stdout.lock();
    if (cert == NULL){

        BIO_printf(this->outbio, "ServerPV: Error: Could not get a certificate \n"
                   /*,hostname*/);
    }
    else{

        BIO_printf(this->outbio, "ServerPV: Retrieved the client's certificate \n"
                   /*,hostname*/);
    }
    pvChiamante->mutex_stdout.unlock();
    /* ---------------------------------------------------------- *
     * extract various certificate information                    *
     * -----------------------------------------------------------*/
    //certname = X509_NAME_new();
    //certname = X509_get_subject_name(cert);

    /* ---------------------------------------------------------- *
     * display the cert subject here                              *
     * -----------------------------------------------------------*/
    //    pvChiamante->mutex_stdout.lock();
    //    BIO_printf(this->outbio, "ServerPV: Displaying the certificate subject data:\n");
    //    X509_NAME_print_ex(this->outbio, certname, 0, 0);
    //    BIO_printf(this->outbio, "\n");
    //    pvChiamante->mutex_stdout.unlock();
    /* ---------------------------------------------------------- *
     * Initialize the global certificate validation store object. *
     * ---------------------------------------------------------- */
    if (!(store = X509_STORE_new())){
        pvChiamante->mutex_stdout.lock();
        BIO_printf(this->outbio, "ServerPV: Error creating X509_STORE_CTX object\n");
        pvChiamante->mutex_stdout.unlock();
    }

    /* ---------------------------------------------------------- *
     * Create the context structure for the validation operation. *
     * ---------------------------------------------------------- */
    vrfy_ctx = X509_STORE_CTX_new();

    /* ---------------------------------------------------------- *
     * Load the certificate and cacert chain from file (PEM).     *
     * ---------------------------------------------------------- */
    int ret;
    /*
         ret = BIO_read_filename(certbio, certFile);
         if (!(cert = PEM_read_bio_X509(certbio, NULL, 0, NULL)))
         BIO_printf(this->outbio, "ServerPV: Error loading cert into memory\n");
         */
    char chainFile[] =
            "/home/giuseppe/myCA/intermediate/certs/ca-chain.cert.pem";

    ret = X509_STORE_load_locations(store, chainFile, NULL);
    if (ret != 1){
        pvChiamante->mutex_stdout.lock();
        BIO_printf(this->outbio, "ServerPV: Error loading CA cert or chain file\n");
        pvChiamante->mutex_stdout.unlock();
    }
    /* ---------------------------------------------------------- *
     * Initialize the ctx structure for a verification operation: *
     * Set the trusted cert store, the unvalidated cert, and any  *
     * potential certs that could be needed (here we set it NULL) *
     * ---------------------------------------------------------- */
    X509_STORE_CTX_init(vrfy_ctx, store, cert, NULL);

    /* ---------------------------------------------------------- *
     * Check the complete cert chain can be build and validated.  *
     * Returns 1 on success, 0 on verification failures, and -1   *
     * for trouble with the ctx object (i.e. missing certificate) *
     * ---------------------------------------------------------- */
    ret = X509_verify_cert(vrfy_ctx);

    pvChiamante->mutex_stdout.lock();
    BIO_printf(this->outbio, "ServerPV: Verification return code: %d\n", ret);
    pvChiamante->mutex_stdout.unlock();

    if (ret == 0 || ret == 1){
        pvChiamante->mutex_stdout.lock();
        BIO_printf(this->outbio, "ServerPV: Verification result text: %s\n",
                   X509_verify_cert_error_string(vrfy_ctx->error));
        pvChiamante->mutex_stdout.unlock();
    }
    /* ---------------------------------------------------------- *
     * The error handling below shows how to get failure details  *
     * from the offending certificate.                            *
     * ---------------------------------------------------------- */
    if (ret == 0) {
        /*  get the offending certificate causing the failure */
        error_cert = X509_STORE_CTX_get_current_cert(vrfy_ctx);
        certsubject = X509_NAME_new();
        certsubject = X509_get_subject_name(error_cert);

        pvChiamante->mutex_stdout.lock();
        BIO_printf(this->outbio, "ServerPV: Verification failed cert:\n");
        X509_NAME_print_ex(this->outbio, certsubject, 0, XN_FLAG_MULTILINE);
        BIO_printf(this->outbio, "\n");
        pvChiamante->mutex_stdout.unlock();
    }

    /* ---------------------------------------------------------- *
     * Free the structures we don't need anymore                  *
     * -----------------------------------------------------------*/
    X509_STORE_CTX_free(vrfy_ctx);
    X509_STORE_free(store);
    X509_free(cert);
    //BIO_free_all(certbio);

    pvChiamante->mutex_stdout.lock();
    cout << "ServerPV: Fine --Verify Client Cert --" << endl;
    pvChiamante->mutex_stdout.unlock();
}

int SSLServer::myssl_fwrite(const char * infile) {
    /* legge in modalità binaria il file e lo strasmette sulla socket aperta
     * una SSL_write per comunicare la lunghezza dello stream da inviare
     * una SSL_write per trasmettere il file binario della lunghezza calcolata
     * */
    ifstream is(infile, std::ifstream::binary);
    if (is) {
        // get length of file:
        is.seekg(0, is.end);
        int length = is.tellg();
        is.seekg(0, is.beg);

        char * buffer = new char[length];
        pvChiamante->mutex_stdout.lock();
        cout << "ServerPV: Reading " << length << " characters... " << endl;
        pvChiamante->mutex_stdout.unlock();
        // read data as a block:
        is.read(buffer, length);

        pvChiamante->mutex_stdout.lock();
        if (is){

            cout << "ServerPV: all characters read successfully." << endl;
        }
        else{
            cout << "ServerPV: error: only " << is.gcount() << " could be read";
        }
        pvChiamante->mutex_stdout.unlock();
        is.close();


        // ...buffer contains the entire file...
        stringstream strs;
        strs << length;
        string temp_str = strs.str();
        const char *info = temp_str.c_str();

        pvChiamante->mutex_stdout.lock();
        cout << "ServerPV: bytes to send:" << info << endl;
        pvChiamante->mutex_stdout.unlock();
        SSL_write(ssl, info, strlen(info));
        SSL_write(ssl, buffer, length);
        delete[] buffer;
        return 1;

    }
    else{
        pvChiamante->mutex_stdout.lock();
        cout << "ServerPV: file unreadable" << endl;
        pvChiamante->mutex_stdout.unlock();
    }
    return 0;
}

void SSLServer::setStopServer(bool b){
    this->stopServer=b;
}

int SSLServer::receiveString_SSL(SSL* ssl, string &s){

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

void SSLServer::sendString_SSL(SSL* ssl, string s) {
    int length = strlen(s.c_str());
    string length_str = std::to_string(length);
    const char *num_bytes = length_str.c_str();
    SSL_write(ssl, num_bytes, strlen(num_bytes));
    SSL_write(ssl, s.c_str(), length);
}
