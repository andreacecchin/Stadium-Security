#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include "dependencies/include/libpq-fe.h"

#define PG_HOST "localhost"
#define PG_USER "postgres" // modificare con il vostro user
#define PG_DB "test" // modificare con il nome del db ( che coincidecon l'user )
#define PG_PASS "" // modificare con la vostra passwordutilizzata per l'accesso in pgadmin
#define PG_PORT 5432

using namespace std ;

PGconn* connecttodb();
int menu();
void checkResults(PGresult* res , const PGconn* conn);
void printquery(PGresult* res);
void printcontrolquery(PGresult* res);
void execquery(PGconn* conn, string query, bool control = false, const char** value = NULL, int npar = 0);


int main ( int argc , char ** argv ) {
    
    PGconn * conn = connecttodb();
    bool a = true;
    while(a){
        int choice = menu();
        PGresult* res;
        string query;
        const char* value[2];
        string par[2];
        switch(choice){
            case 1:
                query = "SELECT r.nome, r.cognome, s.nome AS Stadio, sc.nome AS Societa \
                    FROM responsabile r JOIN stadio s ON (r.stadio=s.codice) JOIN societa sc ON (r.societa=sc.piva);";
                execquery(conn, query);
                break;
            case 2:
                query = "SELECT segnalato.nome AS NomeSegnalato, segnalato.cognome AS CognomeSegnalato,\
                    operatore.nome AS NomeSteward, operatore.cognome AS CognomeSteward, evento.nome AS Evento \
                    FROM segnalazione JOIN segnalato ON (segnalazione.codicesegnalato = segnalato.cf) \
                    JOIN evento ON (segnalazione.codiceevento = evento.codice) \
                    JOIN operatore ON (operatore.cf = segnalazione.codiceoperatore) \
                    WHERE operatore.stipendio IS NOT NULL;";
                execquery(conn, query);
                break;
            case 3:
                query = "SELECT segnalato.nome, segnalato.cognome, stadio.nome AS stadio, s.iniziodaspo, s.finedaspo \
                    FROM segnalazione s JOIN segnalato ON (segnalato.cf = s.codicesegnalato) \
                    JOIN evento e ON(s.codiceevento = e.codice) JOIN stadio ON (stadio.codice = e.stadio) \
                    WHERE s.iniziodaspo IS NOT NULL AND s.finedaspo IS NOT NULL;";
                execquery(conn, query);
                break;
            case 4:
                query = "SELECT e.nome AS evento, SUM(stipendio) AS costooratotale \
                FROM lavorooperatori l \
                    JOIN operatore o ON (l.codiceoperatore = o.cf) \
                    JOIN evento e ON (l.codiceevento = e.codice) \
                WHERE o.ruolo = 'Steward' \
                GROUP BY evento \
                HAVING SUM(stipendio) > 1000 \
                ORDER BY costooratotale ASC;";
                execquery(conn, query);
                break;
            case 5:
                execquery(conn, "SELECT * FROM stadio");
                cout << "Inserisci il codice dello stadio: ";
                cin >> par[0];
                value[0] = par[0].c_str();
                query = "SELECT e.nome AS evento, e.data \
                    FROM evento e JOIN stadio s ON (e.stadio = s.codice) \
                    WHERE s.codice = $1::char(3);";
                execquery(conn, query, false, value, 1) ;
                break;
            case 6:
                execquery(conn, "SELECT * FROM evento");
                cout << "Inserisci il codice dell'evento: ";
                cin >> par[0];
                value[0] = par[0].c_str();
                query = "SELECT o.ruolo, COUNT(l.codiceoperatore) AS num \
                FROM lavorooperatori l \
                    JOIN operatore o ON (l.codiceoperatore = o.cf) \
                    JOIN evento e ON (l.codiceevento = e.codice) \
                WHERE e.codice = $1::integer \
                GROUP BY o.ruolo \
                UNION \
                SELECT 'Pronto Intervento' as ruolo, COUNT(l.codiceoperatore) AS num \
                FROM lavoropi l \
                    JOIN prontointervento o ON (l.codiceoperatore = o.cf) \
                    JOIN evento e ON (l.codiceevento = e.codice) \
                WHERE e.codice = $1::integer \
                GROUP BY ruolo \
                ORDER BY num ASC;";
                execquery(conn, query, false, value, 1);
                break;
            case 7:
                execquery(conn, "SELECT * FROM stadio");
                cout << "Inserisci il codice dello stadio: ";
                cin >> par[0];
                value[0] = par[0].c_str();
                cout << "Inserisci la data durante la quale cercare i daspati (YYYY-MM-DD): ";
                cin >> par[1];
                value[1] = par[1].c_str();
                query = "SELECT s.cf as codicefiscale, s.nome, s.cognome \
                    FROM segnalazione sz JOIN segnalato s ON (sz.codicesegnalato = s.cf) \
                    JOIN evento e ON (sz.codiceevento = e.codice) JOIN stadio ON (e.stadio = stadio.codice) \
                    WHERE stadio.codice = $1::char(3) AND sz.iniziodaspo <= $2::date AND sz.finedaspo >= $2::date;";
                execquery(conn, query, false, value, 2);
                break;
            case 8:
                execquery(conn, "SELECT o.cf, o.nome, o.cognome, o.ruolo FROM operatore o \
                    UNION SELECT o.cf, o.nome, o.cognome, 'Pronto Intervento' as ruolo FROM prontointervento o");
                cout << "Inserisci il codice fiscale dell'operatore: ";
                cin >> par[0];
                value[0] = par[0].c_str();
                query = "SELECT stadio.nome AS stadio, COUNT(e.codice) AS num \
                FROM lavorooperatori l \
                    JOIN evento e ON (l.codiceevento = e.codice) \
                    JOIN stadio ON (e.stadio = stadio.codice) \
                WHERE l.codiceoperatore = $1::char(16) \
                GROUP BY stadio.nome;";
                execquery(conn, query, false, value, 1);
                break;
            case 9:
                query = "SELECT s.codice \
                    FROM segnalazione s JOIN evento e ON (s.codiceevento = e.codice) \
                    WHERE  s.iniziodaspo < e.data;";
                execquery(conn, query, true);
                break;
            case 10:
                query = "SELECT lavorooperatori.codiceoperatore, evento.codice \
                    FROM lavorooperatori JOIN evento ON (lavorooperatori.codiceevento = evento.codice) \
                    JOIN stadio ON (evento.stadio = stadio.codice) \
                    WHERE lavorooperatori.settore>stadio.nsettori \
                    UNION \
                    SELECT lavoropi.codiceoperatore, evento.codice \
                    FROM lavoropi JOIN evento ON (lavoropi.codiceevento = evento.codice) \
                    JOIN stadio ON (evento.stadio = stadio.codice) \
                    WHERE lavoropi.settore>stadio.nsettori;";
                execquery(conn, query, true);
                break;
            case 11: 
                a = false;
                break;
            default:
                cout << "Il numero deve essere tra 1 e 11\n\n";
                break;
        }
    }
    PQfinish(conn);
    return 0;
}

PGconn * connecttodb(){
    char pg_user[50], pg_db[50], pg_pass[50], pg_host[50];
    int pg_port;
    cout << "Inserisci il nome della base di dati: ";
    cin >> pg_db;
    cout << "Inserisci l'host: ";
    cin >> pg_host;
    cout << "Inserisci la porta: ";
    cin >> pg_port;
    cout << "Inserisci lo user: ";
    cin >> pg_user;
    cout << "Inserisci la password: ";
    cin >> pg_pass;
    char conninfo[250];
    sprintf(conninfo, "user=%s password=%s dbname=%s host=%s port=%d", pg_user, pg_pass, pg_db, pg_host, pg_port);
    PGconn * conn = PQconnectdb(conninfo);
    if( PQstatus ( conn ) != CONNECTION_OK ){
        cout <<" Errore di connessione " << PQerrorMessage ( conn );
        PQfinish ( conn );
        exit (1) ;
    }
    else {
        cout <<" Connessione avvenuta correttamente\n";
    }
    return conn;
}

int menu(){
    cout << "+-----------------------------------------------------------------------------------------------------------+\n";
    cout << "|  1. Visualizza gli stadi e le societa' che gli gestiscono con i relativi responsabili                     |\n";
    cout << "|  2. Visualizza tutte le persone segnalate con relativo operatore ed evento                                |\n";
    cout << "|  3. Visualizza le persone soggette a daspo e lo stadio da cui sono state allontanate                      |\n";
    cout << "|  4. Visualizza gli eventi in cui il costo orario totale e' superiore a 1000                               |\n";
    cout << "|  5. Visualizza tutti gli eventi avvenuti in un determinato stadio                                         |\n";
    cout << "|  6. Visualizza quanti operatori per ciascun ruolo hanno lavorato in un determinato evento                 |\n";
    cout << "|  7. Visualizza i daspati da un determinato stadio durante una certa data                                  |\n";
    cout << "|  8. Visualizza gli stadi e il relativo numero di eventi in cui ha lavoroto un determinato operatore       |\n";
    cout << "|  9. Controlla che le date dei daspi siano successive alle date delle rispettive segnalazioni              |\n";
    cout << "| 10. Controlla che i numeri dei settori in cui hanno lavorato gli operatori non siano superiori al numero  |\n";
    cout << "|     di settori del rispettivo stadio                                                                      |\n";
    cout << "| 11. Esci                                                                                                  |\n";
    cout << "+-----------------------------------------------------------------------------------------------------------+\n";
    cout << "Inserisci un numero: ";
    int choice = 0;
    cin >> choice;
    return choice;
}

void execquery(PGconn* conn, string query, bool control, const char** value, int npar){
    PGresult* res = PQexecParams(conn, query.c_str(), npar, NULL, value, 0,0,0);
    checkResults(res , conn);
    if(control) printcontrolquery(res);
    else printquery(res);
    PQclear(res);
}

void checkResults(PGresult * res , const PGconn * conn) {
    if ( PQresultStatus( res ) != PGRES_TUPLES_OK && PQresultStatus( res ) != PGRES_COMMAND_OK )  {
        cout << PQerrorMessage(conn ) << endl ;
        PQclear ( res );
        exit (1) ;
    }
}

void printquery(PGresult* res){
    if(PQntuples(res) == 0){
        cout << "Tabella vuota\n";
    }else{
        cout << endl;
        for(int i = 0; i<PQnfields(res); i++) cout << "+----------------------------";
        cout << "+\n";
        // Stampo intestazioni
        for( int i = 0; i < PQnfields(res) ; ++ i){
            char *s = PQfname(res, i);
            printf("|%*s%*s",14+strlen(s)/2,s,14-strlen(s)/2,"");
        }
        cout << "|\n";

        for(int i = 0; i<PQnfields(res); i++) cout << "+----------------------------";
        cout << "+\n";
        // Stampo i valori selezionati
        for(int i = 0; i < PQntuples(res) ; ++ i){
            for(int j = 0; j < PQnfields(res) ; ++ j){
                char *s = PQgetvalue(res , i, j);
                printf("|%*s%*s",14+strlen(s)/2,s,14-strlen(s)/2,"");
            }
            cout << "|\n" ;
        }
        for(int i = 0; i<PQnfields(res); i++) cout << "+----------------------------";
        cout << "+\n\n";
    }
}

void printcontrolquery(PGresult* res){
    if(PQntuples(res) == 0) cout << "Tutto OK!!" << endl;
    else{
        cout << "Errore!!\n\n";
        printquery(res);
    }
}