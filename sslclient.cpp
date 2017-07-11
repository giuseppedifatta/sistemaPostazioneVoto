#include "sslclient.h"

/*
 * sslclient.h
 *
 *  Created on: 02/apr/2017
 *      Author: giuseppe
 */

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
using namespace std;

#define SERVER_PORT "4433"


SSLClient::SSLClient(){
    this->server_sock = 0;
    /* ---------------------------------------------------------- *
     * Create the Input/Output BIO's.                             *
     * ---------------------------------------------------------- */
    outbio = BIO_new_fp(stdout, BIO_NOCLOSE);
    ssl = nullptr;

    /* ---------------------------------------------------------- *
     * Function that initialize openssl for correct work.		  *
     * ---------------------------------------------------------- */
    this->init_openssl_library();

    const SSL_METHOD *method;
    method = TLSv1_2_client_method();

    /* ---------------------------------------------------------- *
     * Try to create a new SSL context                            *
     * ---------------------------------------------------------- */
    if ((this->ctx = SSL_CTX_new(method)) == NULL)
        BIO_printf(this->outbio, "ClientPV: Unable to create a new SSL context structure.\n");

    /* ---------------------------------------------------------- *
     * Disabling SSLv2 and SSLv3 will leave TSLv1 for negotiation    *
     * ---------------------------------------------------------- */
    SSL_CTX_set_options(this->ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

    char certFile[] = "/home/giuseppe/myCA/intermediate/certs/client.cert.pem";
    char keyFile[] = "/home/giuseppe/myCA/intermediate/private/client.key.pem";
    char chainFile[] =
            "/home/giuseppe/myCA/intermediate/certs/ca-chain.cert.pem";

    this->configure_context(certFile, keyFile, chainFile);
    cout << "ClientPV:Costruttore Client: Cert and key configured" << endl;


}

SSLClient::~SSLClient(){
    /* ---------------------------------------------------------- *
     * Free the structures we don't need anymore                  *
     * -----------------------------------------------------------*/

    SSL_CTX_free(this->ctx);
    BIO_free_all(this->outbio);

    this->cleanup_openssl();

}
void SSLClient::connectTo(const char* hostIP /*hostname*/){
   const char * port = SERVER_PORT;


   int res = create_socket(hostIP /*hostname*/,port);
   //a questo punto la socket del server è stata creata e settata in this->server_sock

   if (res == 0){
       BIO_printf(this->outbio, "ClientPV: Successfully create the socket for TCP connection to: %s.\n",
                  hostIP /*hostname*/);
   }
   else {
       BIO_printf(this->outbio, "ClientPV: Unable to create the socket for TCP connection to: %s.\n",
                  hostIP /*hostname*/);
   }

   /* ---------------------------------------------------------- *
    * Create new SSL connection state object                     *
    * ---------------------------------------------------------- */
   this->ssl = SSL_new(this->ctx);

   //cout << "ClientPV:ConnectTo: " << this->ssl << endl;
   /* ---------------------------------------------------------- *
    * Make the underlying TCP socket connection                  *
    * ---------------------------------------------------------- */

   /* ---------------------------------------------------------- *
    * Attach the SSL session to the socket descriptor            *
    * ---------------------------------------------------------- */

   if (SSL_set_fd(this->ssl, this->server_sock) != 1) {
       BIO_printf(this->outbio, "ClientPV: Error: Connection to %s failed", hostIP /*hostname*/);
   }
   else
       BIO_printf(this->outbio, "ClientPV: Ok: Connection to %s \n", hostIP /*hostname*/);
   /* ---------------------------------------------------------- *
    * Try to SSL-connect here, returns 1 for success             *
    * ---------------------------------------------------------- */
   if (SSL_connect(this->ssl) != 1) //SSL handshake
       BIO_printf(this->outbio, "ClientPV: Error: Could not build a SSL session to: %s.\n",
                  hostIP /*hostname*/);
   else
       BIO_printf(this->outbio, "ClientPV: Successfully enabled SSL/TLS session to: %s.\n",
                  hostIP /*hostname*/);
   ShowCerts();
   verify_ServerCert(hostIP /*hostname*/);
   //SSL_set_connect_state(this->ssl);

   return;
}

void SSLClient::updateStatoPVtoSeggio(const char * hostnameSeggio, unsigned int idPV, unsigned int statoPV){
    //comunica al seggio come è cambiato lo stato della postazione di voto.
    cout << "ClientPV:Try to update..." << endl;

    stringstream ss;
    ss << idPV;
    string str= ss.str();
    const char * charArray_idPV = str.c_str();
    cout << "ClientPV: idPV to update: " << charArray_idPV << endl;

    //cout << strlen(charArray_idPV) << endl;
    SSL_write(ssl, charArray_idPV, strlen(charArray_idPV));

    stringstream ss1;
    ss1 << statoPV;
    const char *  charArray_statoPV = ss1.str().c_str();
    cout << "ClientPV: statoPV to update: " << charArray_statoPV << endl;
    //cout << strlen(charArray_statoPV) << endl;
    SSL_write(ssl, charArray_statoPV, strlen(charArray_statoPV));


    BIO_printf(this->outbio, "ClientPV: Finished SSL/TLS connection with server: %s.\n",
               hostnameSeggio);

    int ret = SSL_shutdown(this->ssl);
    if (ret == 0){
        SSL_shutdown(this->ssl);
    }

    SSL_free(this->ssl);

    if(close(this->server_sock) != 0)
        cerr << "errore chiusura della socket creata per il server" << endl;

}

void SSLClient::init_openssl_library() {

    SSL_library_init();

    SSL_load_error_strings();

    ERR_load_BIO_strings();

    ERR_load_crypto_strings();


    /* OpenSSL_config may or may not be called internally, based on */
    /*  some #defines and internal gyrations. Explicitly call it    */
    /*  *IF* you need something from openssl.cfg, such as a         */
    /*  dynamically configured ENGINE.                              */
    //OPENSSL_config(NULL);

}

int SSLClient::create_socket(const char * hostIP /*hostname*/,const char * port) {
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
    /* decommentare la sezione se si passa alla funzione l'hostname invece dell'ip dell'host
    if ((host = gethostbyname(hostname)) == NULL) {
        BIO_printf(this->outbio, "ClientPV: Error: Cannot resolve hostname %s.\n", hostname);
        abort();
    }
    */

    /* ---------------------------------------------------------- *
     * create the basic TCP socket                                *
     * ---------------------------------------------------------- */
    this->server_sock = socket(AF_INET, SOCK_STREAM, 0);

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(portCod);
    //dest_addr.sin_addr.s_addr = *(long*) (host->h_addr);
    // commentare la riga sotto se non si vuole usare l'ip dell'host, ma l'hostname
    dest_addr.sin_addr.s_addr = inet_addr(hostIP);

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

    if (res  == -1) {
        BIO_printf(this->outbio, "ClientPV: Error: Cannot connect to host %s [%s] on port %d.\n",
                   hostIP /*hostname*/, address_printable, portCod);
    } else {
        BIO_printf(this->outbio, "ClientPV: Successfully connect to host %s [%s] on port %d.\n",
                   hostIP /*hostname*/, address_printable, portCod);

    }

    cout << "ClientPV:Descrittore socket: "<< server_sock << endl;
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

void SSLClient::configure_context(char* CertFile, char* KeyFile, char * ChainFile) {
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
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }

}

void SSLClient::verify_ServerCert(const char * hostIP /*hostname*/) {


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
    if (peer_cert == NULL)
        BIO_printf(this->outbio, "ClientPV: Error: Could not get a certificate from: %s.\n",
                   hostIP /*hostname*/);
    else
        BIO_printf(this->outbio, "ClientPV: Retrieved the server's certificate from: %s.\n",
                   hostIP /*hostname*/);


    // extract various certificate information

    //certname = X509_NAME_new();
    //certname = X509_get_subject_name(peer_cert);

    // display the cert subject here

//    BIO_printf(this->outbio, "ClientPV: Displaying the certificate subject data:\n");
//    X509_NAME_print_ex(this->outbio, certname, 0, 0);
//    BIO_printf(this->outbio, " \n");

    //Initialize the global certificate validation store object.
    if (!(store = X509_STORE_new()))
        BIO_printf(this->outbio, "ClientPV: Error creating X509_STORE_CTX object\n");

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


    if (ret != 1)
        BIO_printf(this->outbio, "ClientPV: Error loading CA cert or chain file\n");

    /* Initialize the ctx structure for a verification operation:
      Set the trusted cert store, the unvalidated cert, and any  *
     * potential certs that could be needed (here we set it NULL) */
    X509_STORE_CTX_init(vrfy_ctx, store, peer_cert, NULL);

    /* Check the complete cert chain can be build and validated.  *
     * Returns 1 on success, 0 on verification failures, and -1   *
     * for trouble with the ctx object (i.e. missing certificate) */
    ret = X509_verify_cert(vrfy_ctx);
    BIO_printf(this->outbio, "ClientPV: Verification return code: %d\n", ret);

    if (ret == 0 || ret == 1)
        BIO_printf(this->outbio, "ClientPV: Verification result text: %s\n",
                   X509_verify_cert_error_string(vrfy_ctx->error));

    /* The error handling below shows how to get failure details  *
     * from the offending certificate.                            */
    if (ret == 0) {
        //get the offending certificate causing the failure
        error_cert = X509_STORE_CTX_get_current_cert(vrfy_ctx);
        certsubject = X509_NAME_new();
        certsubject = X509_get_subject_name(error_cert);
        BIO_printf(this->outbio, "ClientPV: Verification failed cert:\n");
        X509_NAME_print_ex(this->outbio, certsubject, 0, XN_FLAG_MULTILINE);
        BIO_printf(this->outbio, "ClientPV: \n");
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

void SSLClient::stopLocalServer(const char* localhost/*hostname*/){
    //questa funzione contatta il server locale, ma non deve fare alcuna operazione se non quella
    //di sbloccare il server locale dallo stato di attesa di una nuova connessione, così da portare
    //al ricontrollo della condizione del while che se falsa, porta
    //all'interruzione del thread chiamante
    const char * port = SERVER_PORT;

    //la creazione della socket sblocca il server locale dall'accept della connessione tcp

    create_socket(localhost/*hostname*/, port);

    // avendo impostato a true la variabile bool stopServer, non verrà inizializzata la connessione ssl
    // si passa direttamente alla chiusura delle socket
    //seggioChiamante->mutex_stdout.lock();
    cout << "ClientPV: niente da fare... chiudo la socket per il server" << endl;
    //seggioChiamante->mutex_stdout.unlock();
    if(close(this->server_sock) != 0)
    {
            cerr << "ClientSeggio: errore chiusura socket server" << endl;
    }

}


