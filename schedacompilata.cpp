#include "schedacompilata.h"

uint SchedaCompilata::getNonce() const
{
    return nonce;
}

void SchedaCompilata::setNonce(const uint &value)
{
    nonce = value;
}

SchedaCompilata::SchedaCompilata()
{
    
}

void SchedaCompilata::addCandidato(Candidato c)
{
    string matricola = c.getMatricola();
    string nome = c.getNome();
    string cognome = c.getCognome();
    string lista = c.getLista();
    string data = c.getDataNascita();
    string luogo = c.getLuogoNascita();
    SchedaVoto::addCandidato(matricola,nome,cognome,lista,data,luogo);

}
