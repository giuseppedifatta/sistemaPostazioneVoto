#ifndef SSLCLIENT_H
#define SSLCLIENT_H
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <openssl/err.h>

class SSLClient
{
private:

    SSL_CTX *ctx;
    int server_sock;
    BIO * outbio;

    //metodi privati
    void init_openssl_library();
    int create_socket(const char *hostIP, const char * port);
    void ShowCerts();
    void configure_context(char* CertFile, char* KeyFile, char * ChainFile);
    void verify_ServerCert(const char * hostname);
    void cleanup_openssl();
public:
    SSLClient();
    ~SSLClient();
    SSL * ssl;

    SSL * connectTo(const char *hostIP);
    unsigned int getStatoPV();
    void updateStatoPVtoSeggio(const char * hostnamePV, unsigned int idPV, unsigned int statoPV);
};


#endif // SSLCLIENT_H
