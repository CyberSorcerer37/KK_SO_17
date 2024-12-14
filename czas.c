#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h> //ftok //semafor
#include <sys/ipc.h> //ftok //semafor
#include <sys/sem.h> //semafor
#include <time.h> //czas
#include <signal.h> //sygnal
#include <sys/shm.h> //pamiec wspoldzielona

//semidF 0 - zaznacza ze fryzjer jest gotowy do pracy w procesach fryzjera, proces rodzica czeka az wszyscy beda gotowi
//semidF 1 - Po zaznaczeniu swojej gotowosci, fryzjerzy czekaja na zmiane wartosci semafora na liczbe pracownikow
//semidF 2 - semafor dla liczby dostepnych foteli dla fryzjerow
//semidF 3 - semafor otwarcia salonu, jezeli wszyscy fryzjerzy sa gotowi do pracy, salon sie otwiera, pozwalajac losowym klientom wchodzic
//semidF 4 - semafor liczby klientow w poczekalni, jezeli >0 fryzjer bierze klienta
//semidF 5 - semafor liczby  wolnych miejsc w poczekalni, jesli = 0, klient wychodzi
//semidF 6 - semafor liczby fryzjerow gotowych do pracy - info dla klientow
//semidF 7 - semafor pamieci wspoldzielonej

void semafor_p(int semid, int semnum);
void semafor_v(int semid, int semnum);
void semafor_pe(int semid, int semnum);
void semafor_ve(int semid, int semnum);

int pamiec;

typedef struct {
    pid_t pgrp1;
    pid_t pgrp2;
} SharedData;

SharedData *shared;
void przygotuj_pamiec();

int F = 5;    //Liczba fryzjerow
int N = 4;   //Liczba Foteli
int P = 5;  //liczba miejsc w poczekalni
key_t kluczsem1;
int semidC;
int Tp = 8;
int Tk = 10;
int jednostka = 10;
time_t czas_start;
int czas_pracy;

pid_t grupaF;
pid_t grupaK;

int main(){
    pid_t OriginPID = getpid();
    printf("[C] Wlaczono program Czas o PID: %d\n", OriginPID);
    
    kluczsem1 = ftok("/home/inf1s-23z/kasperczyk.krzysztof.152683/p17", 11);
    if(kluczsem1 == -1){
        printf("Nie udalo sie stworzyc klucza dla pierwszego zbioru semaforow!\n");
        exit(EXIT_FAILURE);
    }

    semidC = semget(kluczsem1, 16, IPC_CREAT | 0600);
    if(semidC == -1){
        printf("Nie udalo sie dolaczyc Czasu do zbioru semaforow!\n");
        exit(EXIT_FAILURE);
    }


    printf("Czekam az fryzjerzy i klienci zapisza swoje grupy\n");
    semafor_p(semidC, 9);
    //Wysylamy id grupy do pamieci wspoldzielonej
    przygotuj_pamiec();
    printf("[C] odczytuje grupy klientow i fryzjerow\n");
    grupaF = shared->pgrp1;
    grupaK = shared->pgrp2;
    shmdt(shared);
    semafor_v(semidC, 8);

    printf("[C] Oczekuje na otwarcie salonu istnienie klientow\n");
    semafor_p(semidC, 3);
    semafor_p(semidC, 12);
    printf("[C] Czas zaczyna plynac!\n");
    czas_pracy = (Tk-Tp)*jednostka; //ustala czas pracy
    czas_start = time(NULL);
    while(((time(NULL) - czas_start) < czas_pracy)){
        sleep(1);
    }
    printf("[C] Czas sie zakonczyl! wysylam sygnal do niepracujacych fryzjerow i klientow ktorzy nie sa teraz obslugiwani czyli do\n");
    kill(-grupaF, SIGUSR1);
    kill(-grupaK, SIGUSR1);
    semafor_v(semidC, 14);
    return 0;
}


void semafor_p(int semid, int semnum) {
    struct sembuf operation = {semnum, -1, 0};
    if (semop(semid, &operation, 1) == -1) {
        perror("Nie mozna opuscic semafora");
        exit(EXIT_FAILURE);
    }
}

void semafor_v(int semid, int semnum) {
    struct sembuf operation = {semnum, 1, 0};
    if (semop(semid, &operation, 1) == -1) {
        perror("Nie mozna podniesc semafora");
        exit(EXIT_FAILURE);
    }
}

void semafor_pe(int semid, int semnum) {
    struct sembuf operation = {semnum, -1, IPC_NOWAIT};
    if (semop(semid, &operation, 1) == -1) {
        perror("Nie mozna opuscic semafora");
        exit(EXIT_FAILURE);
    }
}

void semafor_ve(int semid, int semnum) {
    struct sembuf operation = {semnum, 1, IPC_NOWAIT};
    if (semop(semid, &operation, 1) == -1) {
        perror("Nie mozna podniesc semafora");
        exit(EXIT_FAILURE);
    }
}

void przygotuj_pamiec() {
    pamiec = shmget(101, sizeof(SharedData), 0600 | IPC_CREAT);
    if (pamiec == -1) {
        perror("Problemy z utworzeniem pamieci dzielonej");
        exit(EXIT_FAILURE);
    }
    shared = (SharedData *)shmat(pamiec, NULL, 0);
    if (shared == (void *)(-1)) {
        perror("Problem z przydzieleniem adresu");
        exit(EXIT_FAILURE);
    }
}