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

//semidF 0 - zaznacza ze fryzjer jest gotowy do pracy w procach fryzjera, proces rodzica czeka az wszyscy beda gotowi
//semidF 1 - Po zaznaczeniu swojej gotowosci, fryzjerzy czekaja na zmiane wartosci semafora na liczbe pracownikow
//semidF 2 - semafor dla liczby dostepnych foteli dla fryzjerow
//semidF 3 - semafor otwarcia salonu, jezeli wszyscy fryzjerzy sa gotowi do pracy, salon sie otwiera, pozwalajac losowym klientom wchodzic

void semafor_p(int semid, int semnum);
void semafor_v(int semid, int semnum);
int semafor_pe(int semid, int semnum);
void semafor_ve(int semid, int semnum);


int pamiec;

typedef struct {
    pid_t pgrp1;
    pid_t pgrp2;
} SharedData;

SharedData *shared;
void przygotuj_pamiec();


int F = 5;
int N = 4;
int P = 5; //liczba miejsc w poczekalni
key_t kluczsem1;
int semidK;
int Tp = 8;
int Tk = 10;
int jednostka = 1;
time_t czas_start;
int czas_pracy;
int koniec = 0;
int zajety = 0;

void handle_koniec_salonu(int sig){
    printf("[K] Dostalismy syngnal ze salon skonczyl prace\n");
    if(zajety==0){exit(EXIT_SUCCESS);}
    koniec=1;
}
int main(){
    signal(SIGUSR1, &handle_koniec_salonu);
    setpgid(0, 0); // Proces macierzysty zostaje liderem grupy procesów

    kluczsem1 = ftok("/home/inf1s-23z/kasperczyk.krzysztof.152683/p17", 11);
    if(kluczsem1 == -1){
        printf("Nie udalo sie stworzyc klucza dla pierwszego zbioru semaforow!\n");
        exit(EXIT_FAILURE);
    }

    semidK = semget(kluczsem1, 10, IPC_CREAT | 0600);
    if(semidK == -1){
        printf("Nie udalo sie dolaczyc Klientow do zbioru semaforow!\n");
        exit(EXIT_FAILURE);
    }
    
    pid_t OriginPID = getpid();

    printf("[K] Wlaczono program klienci o PID: %d\n", OriginPID);

    printf("[K] Oczekuje na zapisanie przez fryzjerow swojej grupy do pamieci wspoldzielonej\n");
    semafor_p(semidK, 7);
    
    przygotuj_pamiec();
    printf("[K] Zapisuje numer grupy do pamieci wspoldzielonej %d\n", getpgrp());
    shared->pgrp2 = getpgrp();
    shmdt(shared);
    semafor_v(semidK, 9);
    

    printf("[K] Teraz nastapi oczekiwanie az salon sie otworzy (wszyscy fryzjerzy beda gotowi)\n");
    semafor_p(semidK,3);
    printf("[K] Salon sie otworzyl!\n");

    int los = 2;
    czas_pracy = (Tk-Tp)*jednostka; //ustala czas pracy
    czas_start = time(NULL);
    while(1){
        pid_t ForkingPID = fork();
        if(ForkingPID == 0){
            signal(SIGUSR1, &handle_koniec_salonu);
            printf("[K%d] Stworzono klienta - PID: %d, PPID: %d\n",getpid()%1000, getpid(), getppid());
            printf("[K%d] Sprawdzam czy jest dla mnie miejsce w poczekalni\n", getpid()%1000);
            while(semafor_pe(semidK, 5)==-1){
                printf("[K%d] Nie ma dla mnie miejsca w poczekalni, wychodze\n", getpid()%1000);
                exit(EXIT_SUCCESS);
            }
            printf("[K%d] Siadam w poczekalni\n", getpid()%1000);
            semafor_p(semidK, 6);
            semafor_v(semidK, 4);
            semafor_v(semidK, 5);
            zajety=1;
            printf("[K%d] Fryzjer zabiera mnie na strzyzenie\n", getpid()%1000);
            sleep(10);
            printf("[K%d] Wychodze z salonu\n", getpid()%1000);
            semafor_v(semidK, 5);
            exit(EXIT_SUCCESS);

        }
        los = 1 + rand() % (2 -1 +1);
        while(los!=2){
            los = 1 + rand() % (2 -1 +1);
            sleep(1);
        }
    }

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