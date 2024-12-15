#ifndef UTILS_H
#define UTILS_H

extern int F; //Liczba fryzjerow
extern int N; //Liczba Foteli w salonie
extern int P; //Liczba miejsc w poczekalni
extern int Tp; //Godzina otwarcia
extern int Tk; //Godzina zamkniecia
extern int jednostka; //Ile trwa 1 godzina, dla jednostka = 1, godzina trwa 1 sekunde
extern char *fifo_path; //Sciezka do pliku FIFO
extern int koniec; //Zmienna oznaczajaca czy czas pracy salonu sie zakonczyl
extern int zajety; //Zmienna oznaczajaca czy dany proces moze sie zakonczyc od razu, jesli zajety = 1, musi on najpierw skonczyc swoja prace
extern int semid; //Identyfikator semaforow
extern int pamiec; //Identyfikator pamieci wspoldzielonej
extern key_t key; //Klucz do semaforow / pamieci wspoldzielonej itp
extern int fd;
extern int i;

typedef struct {
    pid_t pgrp1;
    pid_t pgrp2;
} SharedData;

extern SharedData *shared; // Deklaracja zmiennej globalnej

void semafor_p(int semid, int semnum);
void semafor_v(int semid, int semnum);
int semafor_pe(int semid, int semnum);
void semafor_ve(int semid, int semnum);
void przygotuj_pamiec();

#endif
