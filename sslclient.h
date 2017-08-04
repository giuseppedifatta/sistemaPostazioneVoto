#ifndef SSLCLIENT_H
#define SSLCLIENT_H
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <string>

#include "postazionevoto.h"

using namespace std;
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


    enum serviziUrna { //richiedente
        attivazionePV = 0, //postazionevoto
        //attivazioneSeggio, //seggio
        //infoProcedura, //seggio
        //infoSessione, //seggio
        //risultatiVoto, //seggio
        invioSchedeCompilate = 5//, //postazionevoto
        //scrutinio, //responsabile procedimento
        //autenticazioneTecnico, //sistema tecnico
        //autenticazioneRP, //responsabile procedimento

    };

    //operazioni del client
    //verso Seggio
    void updateStatoPVtoSeggio(unsigned int idPV, unsigned int statoPV);

    //verso PV stessa
    void stopLocalServer();

    //verso Urna
    bool attivaPostazioneVoto(string sessionKey);
    void inviaSchedeCompilate();

    //verso OTPServerProvider
};


#endif // SSLCLIENT_H
