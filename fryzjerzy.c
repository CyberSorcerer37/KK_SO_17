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
//semidF 8 - semafor
//semidF 10 - zlicza klientow, jak wchodzi +1, jak wychodzi -1, jak 0 = program klientow sie konczy
//semidF 11 - semafor ustawiajacy wartosc na liczbe klientow
//semidF 12 - sygnalizuje czasowi ze klienci tez sa gotowi
//semidF 13 - Oczekiwanie az wszyscy klienci wyjda, zamkniecie salonu
//semidF 14 - klienci sie koncza gdy napewno czas sie zakonczy
//semid 15 - semafor wypuszczajacy fryzjerow jak klienci wyszlo

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
int semidF;
int Tp = 8;
int Tk = 10;
int jednostka = 10;
time_t czas_start;
int czas_pracy;

int koniec = 0;
int zajety = 0;
void handle_koniec_salonu(int sig){
    printf("[F%d] Dostalismy syngnal ze salon skonczyl prace\n", getpid()%100);
    if(zajety==0){
        semafor_p(semidF, 15);
        printf("[F%d] Wyszedlem z salonu dzieki zmiennej zajety\n", getpid()%100); 
        exit(0); 
    }
    koniec=1;
}

void pusty_handler(int sig) {
    
}

int main(){
    signal(SIGUSR1, &pusty_handler);
    pid_t OriginPID = getpid();
    printf("[F] Wlaczono program fryzjerzy o PID: %d\n", OriginPID);
    sleep(2);
    setpgid(0, 0); // Proces macierzysty zostaje liderem grupy procesów

    kluczsem1 = ftok("/home/inf1s-23z/kasperczyk.krzysztof.152683/p17", 11);
    if(kluczsem1 == -1){
        printf("Nie udalo sie stworzyc klucza dla pierwszego zbioru semaforow!\n");
        exit(EXIT_FAILURE);
    }

    semidF = semget(kluczsem1, 16, IPC_CREAT | 0600);
    if(semidF == -1){
        printf("Nie udalo sie dolaczyc Fryzjerow do zbioru semaforow!\n");
        exit(EXIT_FAILURE);
    }


    //Wysylamy id grupy do pamieci wspoldzielonej
    przygotuj_pamiec();
    printf("[F] Zapisuje numer grupy do pamieci wspoldzielonej %d\n", getpgrp());
    shared->pgrp1 = getpgrp();
    shmdt(shared);
    semafor_v(semidF, 7);
    sleep(2);

    
    if(semctl(semidF, 2, SETVAL, N) == -1){
        printf("Nie udalo sie zmienic wartosci semafora 2 na liczbe foteli\n");
        exit(EXIT_FAILURE);
    }

    if(semctl(semidF, 5, SETVAL, P) == -1){
        printf("Nie udalo sie zmienic wartosci semafora 4 na liczbe miejsc w poczekalni\n");
        exit(EXIT_FAILURE);
    }

    
    sleep(1);
    printf("[F] Za chwile nastapi stworzenie %d fryzjerow (procesy potomne) GRUPA: %d\n", F, getpgrp());
    sleep(2);
    int i;
    pid_t ForkingPID;
    for(i = 0; i<F ; i++){
        ForkingPID = fork();
        if(ForkingPID == -1){
            printf("Nie udalo sie stworzyc procesu potomnego o nr %d\n", i);
            exit(EXIT_FAILURE);
        }
        //Sekcja pracy fryzjerow
        else if(ForkingPID == 0){
            signal(SIGUSR1, &handle_koniec_salonu);
            pid_t FryzjerPID = getpid();
            printf("[F%d] Stworzono fryzjera - PID: %d, PPID: %d GRUPA: %d\n", FryzjerPID%100, getpid(), getppid(), getpgrp());
            semidF = semget(kluczsem1, 16, IPC_CREAT | 0600);
            if(semidF == -1){
                printf("Nie udalo sie dolaczyc fryzjera do zbioru semaforow!\n");
                exit(EXIT_FAILURE);
            }

            semafor_v(semidF, 0); //Zaznacza ze fryzjer jest gotowy do pracy
            semafor_p(semidF, 1);
            czas_pracy = (Tk-Tp)*jednostka; //ustala czas pracy
            czas_start = time(NULL);
            while(koniec == 0){
                zajety=0;
                semafor_p(semidF,2);
                printf("[F%d] Zabralem wolny fotel...\n", FryzjerPID%100);
                printf("[F%d] Zaznaczylem ze jestem gotowy do pracy\n", FryzjerPID%100);
                semafor_v(semidF, 6);  
                semafor_p(semidF, 4);
                zajety=1;
                printf("[F%d] Wzialem klienta z poczekalni\n", FryzjerPID%100);
                sleep(2);
                semafor_v(semidF, 2);
                zajety=0;
                printf("[F%d] Zwolnilem fotel...\n", FryzjerPID%100);
            }
            semafor_p(semidF, 15);
            printf("[F%d] Wyszedlem z salonu dzieki zmiennej koniec\n", getpid()%100);
            exit(0);
        }
        //Sekcja pracy fryzjerow
    }
    
    //Czeka az wszyscy fryzjerzy beda gotowi do pracy
    for(i=0;i<F;i++){
        semafor_p(semidF, 0);
    }
    czas_pracy = (Tk-Tp)*jednostka; //ustala czas pracy
    printf("Wszyscy fryzjerzy zglosili gotowosc do pracy !!!\n");
    sleep(2);
    printf("Salon bedzie pracowal od %d do %d, przez %d sekund = %f minut = %f godzin\n", Tp, Tk, czas_pracy, czas_pracy/60, czas_pracy/3600);

    semafor_p(semidF, 8);
    printf("3...\n");
    sleep(1);
    printf("2...\n");
    sleep(1);
    printf("1...\n");
    sleep(1);

    if(semctl(semidF, 1, SETVAL, F) == -1){
        printf("Nie udalo sie zmienic wartosci semafora 1 na 4\n");
        exit(EXIT_FAILURE);
    }
    while(semctl(semidF, 2, GETVAL) != 0){
        sleep(1);
    }
    if(semctl(semidF, 3, SETVAL, 2) == -1){  //Podniesienie semaforu, sygnalizuje ze salon jest gotowy, klienci moga zaczac sie pojawiac
        printf("Nie udalo sie zmienic wartosci semafora 1 na 4\n");
        exit(EXIT_FAILURE);
    } 
    printf("[F] Salon jest gotowy!\n");
    for(i=0; i<F ; i++){
        pid_t KoniecFryzjerPID = wait(NULL);
        if(KoniecFryzjerPID == -1){
            printf("[F] %d fryzjer nie zakonczyl swojej pracy!\n", i+1);
            exit(EXIT_FAILURE);
        }
        else{
            printf("[F] %d fryzjer zakonczyl swoja prace\n", i+1);
        }
    }
    semafor_p(semidF, 13); // Salon sie zamyka gdy wszyscy klienci wyjda
    printf("[F] Zamkniecie salonu!\n");
    shmctl(pamiec, IPC_RMID, NULL);
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