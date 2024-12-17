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
    printf("[F%d] Dostalem syngnal ze salon skonczyl prace\n", getpid()%100);
    if(zajety==0){
        exit(0); 
    }
    koniec=1;
}
void handle_koniec_salonu_rodzic(int sig){
    printf("[F] Fryzjerzy powinni zaczac konczyc prace\n");
}

// Semafor 0 - liczba foteli, Rodzic fryzjerow ustawia na liczbe foteli, potem fryzjerzy w petli je zabieraja
// Semafor 1 - liczba miejsc w poczekalni, Rodzic fryjzerow ustawia na liczbe miejsc, potem klienci wchodzacy do salonu w petli zmniejszaja ta wartosc
// Semafor 2 - Umozliwia otworzenie pliku fifo tylko przez jednego fryzjera
// Semafor 3 - blokuje fryzjerow przed zaczeciem pracy do momentu az rodzic bedzie czekal na ich zakonczenie
// Semafor 4 - Gdy menadzer odczyta pidy swoich pracownikow pozwala fryzjerowi czekac na ich zakonczenie a fryzjer pozwala pracowac
// Semafor 5 - podnoszony u fryzjerow po zapisaniu swoich grup do pamieciu wspoldzielonej zeby klient mogl zapisac swoja
// Semafor 6 - Semafor zakonczenia programu fryzjerzy, jezeli wszystko we fryzjerach sie zakonczy, podnoszony jest semafor i menadzer tez konczy prace
// Semafor 7 - Liczba fryzjerow czekajacych na klienta
// Semafor 8 - Liczba klientow czekajacych na fryzjera
// Semafor 9 - podnoszony u klienta po zapisaniu swoich grup do pamieciu wspoldzielonej zeby menadzer mogl odczytac
// Semafor 10 - podnoszony gdy fryzjerzy sa gotowi do pracy, rodzicowi klientow zaczac czekac na zakonczenie dzieci
// Semafor 11 - wlaczany od razu po semaforze 10, pozwala klientom zaczac sie tworzyc
// Semafor 12 - gdy wszyscy klienci opuszcza salon, pozwala wszystkim fryzjerom opuscic salon
// Semafor 13 - podnosi sie za kazdym razem gdy tworzony jest klient, jesli stworza sie wszyscy klienci, czas dzialania salonu sie odpala
// Semafor 14 - gdy czas dzialania salonu sie wlaczy, pozwala on wchodzic klientom
int main(){
    signal(SIGUSR1, handle_koniec_salonu_rodzic);
    pid_t OriginPID = getpid(); //Pobranie PIDu
    pid_t ForkingPID;          //PID rozpoznajacy rodzica lub dziecko
    pid_t FryzjerPID;         //PID dziecka                    
    
    printf("[F] Wlaczono program fryzjerzy o PID: %d\n", OriginPID);            //Wyświetlenie go
    
    setpgid(0,0);  //Ustawienie PIDu grupy procesów na rodzica, każdy potomek odiedziczy tą grupe (do wysylania sygnalu do kazdego fryzjera)

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
    

    przygotuj_pamiec();    //Otwarcie pamieci wspoldzielonej
    printf("[F] Zapisuje numer grupy do pamieci wspoldzielonej %d\n", getpgrp());
    shared->pgrp1 = getpgrp();    //Zapisanie grupy do pamieci
    shmdt(shared);               //Odlaczenie od pamieci wspoldzielonej
    semafor_v(semid, 5);        // Podnosi semafor zeby klient mogl wpisac swoja grupe

    //Ustawienie semafora 0 na liczbe foteli
    if(semctl(semid, 0, SETVAL, N) == -1){
        printf("[F] Nie udalo sie zmienic wartosci semafora 1 na liczbe foteli\n");
        exit(EXIT_FAILURE);
    }
    

    if(semctl(semid, 1, SETVAL, P) == -1){
        printf("[F] Nie udalo sie zmienic wartosci semafora 1 na liczbe miejsc w poczekalni\n");
        exit(EXIT_FAILURE);
    }

    sleep(2);
    printf("[F] Za chwile nastapi stworzenie %d fryzjerow (procesy potomne) GRUPA: %d\n", F, getpgrp());
    sleep(2);
    for(i = 0; i<F ; i++){
        ForkingPID = fork();
        if(ForkingPID == -1){
            printf("[F] Nie udalo sie stworzyc procesu potomnego o nr %d\n", i);
            exit(EXIT_FAILURE);
        }
        else if(ForkingPID == 0){
            
            signal(SIGUSR1, handle_koniec_salonu);
            FryzjerPID = getpid();
            printf("[F%d] Stworzono fryzjera - PID: %d, PPID: %d GRUPA: %d\n", FryzjerPID%100, getpid(), getppid(), getpgrp());

            semid = semget(key, 20, IPC_CREAT | 0600);
            if(semid == -1){
                printf("[F] Nie udalo sie dolaczyc do zbioru semaforow!\n");
                exit(EXIT_FAILURE);
            }

            semafor_p(semid, 2);
            fd = open("fryzjerzy_fifo", O_WRONLY); 
            if (fd == -1) {
                perror("[F] Nie udało się otworzyć FIFO");
                return 1;
            }
            if (dprintf(fd, "%d\n", getpid()) < 0) {
                perror("[F] Nie udało się wysłać PID do FIFO");
            }
            
            semafor_p(semid, 3);

            while(koniec == 0){
                semafor_p(semid, 0);
                printf("[F%d] Zabralem wolny fotel, czekam na klienta\n", getpid()%100);
                semafor_v(semid, 7); //Daje znac klientowi ze jestem gotowy
                semafor_p(semid, 8); //Czekam na gotowego klienta
                zajety = 1;
                printf("[F%d] Zabieram klienta na strzyzenie\n", getpid()%100);
                sleep(2);
                printf("[F%d] Zakonczylem strzyzenie\n", getpid()%100);
                zajety = 0;
            }
            
            exit(EXIT_SUCCESS);
        }
    }

    semafor_p(semid, 4);
    if(semctl(semid, 3, SETVAL, F) == -1){ 
        printf("[F] Nie udalo sie zmienic wartosci semafora 3 na liczbe fryzjerow \n");
        exit(EXIT_FAILURE);
    }
        //Tu fryzjerzy sa gotowi do pracy, a w zasadzie zaczeli czekac na klientow
    while(semctl(semid, 7, GETVAL) != N){ //Sprawdzam czy fryzjerzy sa gotowi, jesli tak to wlaczam semafor ktory pozwoli zaczac sie tworzyc klientom;
        sleep(1);
    }
    semafor_v(semid, 10); // Klienci moga zaczac sie tworzyc
        
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
    semafor_p(semid,12); //Czeka az wszyscy klienci opuszcza salon

    printf("[F] Wszyscy fryzjerzy zakonczyli prace!\n");
    semafor_v(semid, 6);

    




    return 0;
}