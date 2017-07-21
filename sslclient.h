#ifndef SSLCLIENT_H
#define SSLCLIENT_H
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <openssl/err.h>

#include "postazionevoto.h"
class PostazioneVoto;

class SSLClient
{
private:


    //dati membro per la connessione SSL
    SSL_CTX *ctx;
    int server_sock;
    BIO * outbio;
    SSL * ssl;
    const char * hostIPAddress; //indirizzo IP del seggio o dell'urna

    PostazioneVoto *pvChiamante;

    //metodi privati
    void init_openssl_library();
    void createClientContext();
    int create_socket(const char * port);
    void ShowCerts();
    void configure_context(char* CertFile, char* KeyFile, char * ChainFile);
    void verify_ServerCert();
    void cleanup_openssl();

    public:
    SSLClient(PostazioneVoto * pv);
    ~SSLClient();


    SSL *connectTo(const char *hostIP);
    unsigned int getStatoPV();
    void updateStatoPVtoSeggio(unsigned int idPV, unsigned int statoPV);
    void stopLocalServer();

};


#endif // SSLCLIENT_H
