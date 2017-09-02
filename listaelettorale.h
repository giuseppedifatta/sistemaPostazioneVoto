#ifndef LISTAELETTORALE_H
#define LISTAELETTORALE_H
#include "candidato.h"
#include <vector>
using namespace std;

class ListaElettorale
{
public:
    ListaElettorale();
    ListaElettorale(string nome, uint numeroLista);
    string getNome() const;
    void setNome(const string &value);
    void addCandidato(Candidato &candidato);
    bool removeCandidato(string matrCandidato);

    vector<Candidato> getCandidati() const;

    uint getNumeroLista() const;
    void setNumeroLista(const uint &value);

private:
    string nome;
    vector <Candidato> candidati;
    uint numeroLista;
};

#endif // LISTAELETTORALE_H
