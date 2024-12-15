#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h> 
#include <sys/ipc.h> 
#include <sys/sem.h> 
#include <time.h> 
#include <signal.h> 
#include <sys/shm.h> 
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "utils.h"

int F = 5; //Fryzjerzy
int N = 4; //Fotele
int K = 50; //Liczba klientow
int P = 5; //Liczba miejsc w poczekalni
int Tp = 8; //Godzina otwarcia
int Tk = 16; //Godzina zamkniecia
int jednostka = 2; //Czas trwania jednej godziny w sekundach
char *fifo_path = "fryzjerzy_fifo";
int koniec = 0;
int zajety = 0;
int pamiec;
int semid;
SharedData *shared; 
key_t key;
int fd;
int i;


int losowa_liczba(int a, int b) {
    return a + rand() % (b - a + 1);
}

void semafor_p(int semid, int semnum) {
    struct sembuf operation = {semnum, -1, 0};
    if (semop(semid, &operation, 1) == -1) {
        perror("[K] Nie mozna opuscic semafora");
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
int semafor_pe(int semid, int semnum) {
    struct sembuf operation = {semnum, -1, IPC_NOWAIT};
    if (semop(semid, &operation, 1) == -1) {
        return -1; // Zwróć -1, jeśli operacja się nie powiodła
    }
    return 0; // Zwróć 0, jeśli operacja się powiodła
}

void semafor_ve(int semid, int semnum) {
    struct sembuf operation = {semnum, 1, IPC_NOWAIT};
    if (semop(semid, &operation, 1) == -1) {
        perror("Nie mozna podniesc semafora");
        exit(EXIT_FAILURE);
    }
}

void przygotuj_pamiec() {
    pamiec = shmget(key, sizeof(SharedData), 0600 | IPC_CREAT);
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
