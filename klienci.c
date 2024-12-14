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
#include <errno.h>
#include <fcntl.h>

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
int K = 50;
int N = 4;
int P = 5; //liczba miejsc w poczekalni
key_t kluczsem1;
int semidK;
int Tp = 8;
int Tk = 10;
int jednostka = 5;
time_t czas_start;
int czas_pracy;
int koniec = 0;
int zajety = 0;


void handle_koniec_salonu(int sig){
    
    if(zajety==0){
        printf("[K%d] Dostalem syngnal ze salon skonczyl prace, wychodze bo nie bylem obslugiwany\n", getpid()%1000);
        exit(EXIT_SUCCESS);
    }
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

    semidK = semget(kluczsem1, 17, IPC_CREAT | 0600);
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
    zajety=1;
    int i;
    for(i=0;i<K;i++){
        pid_t ForkingPID = fork();
        if(ForkingPID == 0){
            signal(SIGUSR1, &handle_koniec_salonu);
            printf("[K%d] Stworzono klienta - PID: %d, PPID: %d\n",getpid()%1000, getpid(), getppid());
            semafor_v(semidK, 10); //zaznacza ze klient jest gotowy
            semafor_p(semidK, 11); //czeka az glowny program bedzie gotowyjobs
            while(1+ rand () % (50 - 1+1)!=50){
                sleep(1);
            }
            while(koniec == 0){
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
                sleep(2);
                semafor_v(semidK, 5);
                zajety=0;
                printf("[K%d] Wychodze z salonu\n", getpid()%1000);
                sleep(1 + rand() % (3 -1 +1));
            }
            printf("[K%d] Dostalem syngnal ze salon skonczyl prace, wychodze po zakonczonym strzyzeniu\n", getpid()%1000);
            exit(EXIT_SUCCESS);
        }
    }
    for(i=0;i<K;i++){
        semafor_p(semidK, 10);
    }
    printf("Klienci sa gotowi\n");
    semafor_v(semidK, 12); //sygnalizuje czasowi ze klienci tez sa gotowi
    if(semctl(semidK, 11, SETVAL, K) == -1){
        printf("Nie udalo sie zmienic wartosci semafora 11 na 50\n");
        exit(EXIT_FAILURE);
    }

    for(i=0; i<K ; i++){
        pid_t KoniecKlientaPID = wait(NULL);
        if(KoniecKlientaPID == -1){
            printf("[K] %d klient nie zakonczyl swojej pracy!\n", i+1);
            exit(EXIT_FAILURE);
        }
        /*else{
            printf("[K] %d Klient zakonczyl swoja prace\n", i+1);
        }*/
    }
    semafor_p(semidK, 14); // klienci sie koncza gdy napewno czas sie zakonczy
    printf("Wszyscy klienci opuscili salon, Program tworzacy klientow sie zakonczyl\n");
    semctl(semidK, 15, SETVAL, F);
    semafor_v(semidK, 13);
    return 0;
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