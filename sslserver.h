#ifndef SSLSERVER_H
#define SSLSERVER_H

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/conf.h>
#include <openssl/x509.h>
#include <openssl/buffer.h>
#include <openssl/x509v3.h>
#include <openssl/opensslconf.h>

#include <vector>
#include <mutex>

#include <thread>
#include <queue>
#include "postazionevoto.h"
#include "conf.h"

using namespace std;

class PostazioneVoto;

class SSLServer{
public:

    SSLServer(PostazioneVoto *pv);
    ~SSLServer();
    void ascoltaSeggio();
    void setStopServer(bool b);

    enum servizi {
        setAssociation,
        pullPVState,
        removeAssociation,
        freePV
    };
    PostazioneVoto * pvChiamante;

private:

    BIO* outbio;
    SSL_CTX * ctx;
    SSL * ssl;
    int listen_sock;

    bool stopServer;

    int openListener(int s_port);
    void init_openssl_library();
    void cleanup_openssl();

    void createServerContext();
    void configure_context(const char *CertFile, const char *KeyFile, const char *ChainFile);
    void ShowCerts(SSL *ssl);
    void verify_ClientCert(SSL *ssl);
    void Servlet(int client_sock);
    void service(SSL *ssl, servizi servizio);
    int myssl_fwrite(const char * infile);

    void print_error_string(unsigned long err, const char* const label);
    int verify_callback(int preverify, X509_STORE_CTX* x509_ctx);
    void print_san_name(const char* label, X509* const cert);
    void print_cn_name(const char* label, X509_NAME* const name);

    void sendString_SSL(SSL *ssl, string s);
    int receiveString_SSL(SSL *ssl, string &s);
};

#endif // SSLSERVER_H
