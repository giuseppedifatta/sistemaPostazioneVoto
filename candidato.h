#ifndef CANDIDATO_H
#define CANDIDATO_H


#include <string>
using namespace std;
class Candidato
{
public:
    Candidato(string nome, string lista, string cognome,string data, string luogo,string matricola);

    string getLista() const;
    void setLista(const string &value);

    string getNome() const;
    void setNome(const string &value);

    string getCognome() const;
    void setCognome(const string &value);

    string getLuogoNascita() const;
    void setLuogoNascita(const string &value);

    string getDataNascita() const;
    void setDataNascita(const string &value);

    string getMatricola() const;
    void setMatricola(const string &value);

private:
    string lista;
    string nome;
    string cognome;
    string luogoNascita;
    string dataNascita;
    string matricola;


};

#endif // CANDIDATO_H
