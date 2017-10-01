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

uint SchedaCompilata::getIdSeggio() const
{
    return idSeggio;
}

void SchedaCompilata::setIdSeggio(const uint &value)
{
    idSeggio = value;
}

string SchedaCompilata::getDescrizioneElezione() const
{
    return descrizioneElezione;
}

void SchedaCompilata::setDescrizioneElezione(const string &value)
{
    descrizioneElezione = value;
}

SchedaCompilata::SchedaCompilata()
{
    idSeggio = 0; // da aggiornare con l'idSeggio effettivo di appartenenza della postazione di voto all'atto di creazione del file xml
}

