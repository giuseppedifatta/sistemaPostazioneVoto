#ifndef SCHEDACOMPILATA_H
#define SCHEDACOMPILATA_H
#include "schedavoto.h"

class SchedaCompilata:public SchedaVoto
{
private:
    uint nonce;
public:
    SchedaCompilata();
    void addCandidato(Candidato c);
    uint getNonce() const;
    void setNonce(const uint &value);
};

#endif // SCHEDACOMPILATA_H
