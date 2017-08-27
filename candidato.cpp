#include "candidato.h"

Candidato::Candidato(string nome, string lista, string cognome, string data, string luogo, string matricola)
{
    this->nome = nome;
    this->cognome = cognome;
    this->dataNascita = data;
    this->luogoNascita = luogo;
    this->lista = lista;
    this->matricola = matricola;
}


std::string Candidato::getLista() const
{
    return lista;
}

void Candidato::setLista(const std::string &value)
{
    lista = value;
}

string Candidato::getNome() const
{
    return nome;
}

void Candidato::setNome(const string &value)
{
    nome = value;
}

string Candidato::getCognome() const
{
    return cognome;
}

void Candidato::setCognome(const string &value)
{
    cognome = value;
}

string Candidato::getLuogoNascita() const
{
    return luogoNascita;
}

void Candidato::setLuogoNascita(const string &value)
{
    luogoNascita = value;
}

string Candidato::getDataNascita() const
{
    return dataNascita;
}

void Candidato::setDataNascita(const string &value)
{
    dataNascita = value;
}

string Candidato::getMatricola() const
{
    return matricola;
}

void Candidato::setMatricola(const string &value)
{
    matricola = value;
}

