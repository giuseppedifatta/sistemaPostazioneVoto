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
};

#endif // SCHEDACOMPILATA_H
