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

void handle_koniec_salonu(int sig){
    printf("[K%d] Dostalem syngnal ze salon skonczyl prace\n", getpid()%1000);
    if(zajety==0){
        exit(0); 
    }
    koniec=1;
}
void handle_koniec_salonu_rodzic(int sig){
    printf("[K] Klienci powinni zaczac opuszczac salon\n");
}

int main(){

    signal(SIGUSR1, handle_koniec_salonu_rodzic);
    pid_t OriginPID = getpid(); //Pobranie PIDu
    pid_t ForkingPID;          //PID rozpoznajacy rodzica lub dziecko
    pid_t KlientPID;         //PID dziecka

    printf("[K] Wlaczono program klienci o PID: %d\n", OriginPID);            //Wyświetlenie go
    
    setpgid(0,0);  //Ustawienie PIDu grupy procesów na rodzica, każdy potomek odiedziczy tą grupe (do wysylania sygnalu do kazdego klienta)

    key = ftok("/home/krzys/p17", 11);
    if(key == -1){
        printf("[F] Nie udalo sie stworzyc klucza dla zbioru semaforow i pamieci wspoldzielonej\n");
        exit(EXIT_FAILURE);
    }

    semid = semget(key, 20, IPC_CREAT | 0600);
    if(semid == -1){
        printf("[F] Nie udalo sie dolaczyc do zbioru semaforow!\n");
        exit(EXIT_FAILURE);
    }

    semafor_p(semid, 5);
    przygotuj_pamiec();    //Otwarcie pamieci wspoldzielonej
    printf("[K] Zapisuje numer grupy do pamieci wspoldzielonej %d\n", getpgrp());
    shared->pgrp2 = getpgrp();    //Zapisanie grupy do pamieci
    shmdt(shared);               //Odlaczenie od pamieci wspoldzielonej
    semafor_v(semid, 9);

    

    for(i = 0; i<K ; i++){
        ForkingPID = fork();
        if(ForkingPID == -1){
            printf("[K] Nie udalo sie stworzyc procesu potomnego o nr %d\n", i);
            exit(EXIT_FAILURE);
        }
        else if(ForkingPID == 0){
            while(semctl(semid, 11, GETVAL)!=1){
                sleep(1);
            }
            signal(SIGUSR1, handle_koniec_salonu);
            printf("[K%d] Stworzono klienta - PID: %d, PPID: %d GRUPA: %d\n", getpid()%1000, getpid(), getppid(), getpgrp());
            semafor_v(semid, 13);
            semafor_p(semid, 14);
            sleep(losowa_liczba(1,3));
            while(koniec == 0){
                printf("[K%d] Sprawdzam czy jest dla mnie miejsce w poczekalni\n", getpid()%1000);
                while(semafor_pe(semid, 1)==-1){
                    printf("[K%d] Nie ma dla mnie miejsca w poczekalni, wychodze\n", getpid()%1000);
                    sleep(losowa_liczba(4,8));
                    printf("[K%d] Sprawdzam czy jest dla mnie miejsce w poczekalni\n", getpid()%1000);
                }
                printf("[K%d] Siadam w poczekalni\n", getpid()%1000);
                semafor_v(semid, 8); //Daje znac ze jestem gotowy
                semafor_p(semid, 7); //Zabieram gotowego fryzjera
                zajety = 1;
                printf("[K%d] Fryzjer zabral mnie na strzyzenie\n", getpid()%1000);
                sleep(2);
                printf("[K%d] Schodze z fotela\n", getpid()%1000);
                semafor_v(semid, 0);
                zajety = 0;
                sleep(losowa_liczba(1,3));
            }
            exit(EXIT_SUCCESS);
        }
    }

    semafor_p(semid,10);
    semafor_v(semid, 11); //Daje znak dzieciom ze moga zaczynac swoja prace
    for(i=0; i<K ; i++){
        pid_t KoniecKlientPID = wait(NULL);
        if(KoniecKlientPID == -1){
            printf("[K] %d Klient nie zakonczyl swojej pracy!\n", i+1);
            exit(EXIT_FAILURE);
        }
        else{
            printf("[K] %d Klient zakonczyl swoje dzialanie\n", i+1);
        }
    }
    printf("[K] Wszyscy klienci opuscili salon!\n");
    semafor_v(semid, 12);
    return 0;
}