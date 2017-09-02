#include "schedacompilata.h"

uint SchedaCompilata::getNonce() const
{
    return nonce;
}

void SchedaCompilata::setNonce(const uint &value)
{
    nonce = value;
}

uint SchedaCompilata::getIdProcedura() const
{
    return idProcedura;
}

void SchedaCompilata::setIdProcedura(const uint &value)
{
    idProcedura = value;
}

uint SchedaCompilata::getNumPreferenze() const
{
    return numPreferenze;
}

void SchedaCompilata::setNumPreferenze(const uint &value)
{
    numPreferenze = value;
}

uint SchedaCompilata::getTipologiaElezione() const
{
    return tipologiaElezione;
}

void SchedaCompilata::setTipologiaElezione(const uint &value)
{
    tipologiaElezione = value;
}

void SchedaCompilata::addMatricolaPreferenza(string matricolaPreferenza)
{
    matricolePreferenze.push_back(matricolaPreferenza);
}

vector<string> SchedaCompilata::getMatricolePreferenze() const
{
    return matricolePreferenze;
}

uint SchedaCompilata::getIdScheda() const
{
    return idScheda;
}

void SchedaCompilata::setIdScheda(const uint &value)
{
    idScheda = value;
}

SchedaCompilata::SchedaCompilata()
{
    
}

