#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h> //ftok //semafor
#include <sys/ipc.h> //ftok //semafor
#include <sys/sem.h> //semafor
#include <time.h> //czas

//semidF 0 - zaznacza ze fryzjer jest gotowy do pracy w procach fryzjera, proces rodzica czeka az wszyscy beda gotowi
//semidF 1 - Po zaznaczeniu swojej gotowosci, fryzjerzy czekaja na zmiane wartosci semafora na liczbe pracownikow
//semidF 2 - semafor dla liczby dostepnych foteli dla fryzjerow
//semidF 3 - semafor otwarcia salonu, jezeli wszyscy fryzjerzy sa gotowi do pracy, salon sie otwiera, pozwalajac losowym klientom wchodzic

void semafor_p(int semid, int semnum);
void semafor_v(int semid, int semnum);

int F = 5;
int N = 4;
key_t kluczsem1;
int semidF;
int Tp = 8;
int Tk = 10;
int jednostka = 1;
time_t czas_start;
int czas_pracy;

int main(){
    kluczsem1 = ftok("/home/inf1s-23z/kasperczyk.krzysztof.152683/p17", 11);
    if(kluczsem1 == -1){
        printf("Nie udalo sie stworzyc klucza dla pierwszego zbioru semaforow!\n");
        exit(EXIT_FAILURE);
    }

    semidF = semget(kluczsem1, 4, IPC_CREAT | 0600);
    if(semidF == -1){
        printf("Nie udalo sie dolaczyc Fryzjerow do zbioru semaforow!\n");
        exit(EXIT_FAILURE);
    }
    
    if(semctl(semidF, 2, SETVAL, N) == -1){
        printf("Nie udalo sie zmienic wartosci semafora 2 na liczbe foteli\n");
        exit(EXIT_FAILURE);
    }

    pid_t OriginPID = getpid();
    printf("[F] Wlaczono program klienci o PID: %d\n", OriginPID);
    sleep(1);
    printf("[F] Za chwile nastapi stworzenie %d fryzjerow (procesy potomne)\n", F);
    sleep(1);
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
            pid_t FryzjerPID = getpid();
            printf("[F%d] Stworzono fryzjera - PID: %d, PPID: %d\n", FryzjerPID%100, getpid(), getppid());
            semidF = semget(kluczsem1, 3, IPC_CREAT | 0600);
            if(semidF == -1){
                printf("Nie udalo sie dolaczyc fryzjera do zbioru semaforow!\n");
                exit(EXIT_FAILURE);
            }

            semafor_v(semidF, 0); //Zaznacza ze fryzjer jest gotowy do pracy
            semafor_p(semidF, 1);
            czas_pracy = (Tk-Tp)*jednostka; //ustala czas pracy
            czas_start = time(NULL);
            while(time(NULL) - czas_start < czas_pracy){
                semafor_p(semidF,2);
                printf("[F%d] Zabralem wolny fotel...\n", FryzjerPID%100);
                sleep(1);
                semafor_v(semidF, 2);
                printf("[F%d] Zwolnilem fotel...\n", FryzjerPID%100);
            }
            
            sleep(1);
            printf("[F%d] Fryzjer konczy prace\n", FryzjerPID%100);
            exit(EXIT_SUCCESS);
        }
        //Sekcja pracy fryzjerow
    }
    
    //Czeka az wszyscy fryzjerzy beda gotowi do pracy
    for(i=0;i<F;i++){
        semafor_p(semidF, 0);
    }
    czas_pracy = (Tk-Tp)*jednostka; //ustala czas pracy
    printf("Wszyscy fryzjerzy zglosili gotowosc do pracy !!!\n");
    printf("Salon bedzie pracowal od %d do %d, przez %d sekund = %f minut = %f godzin\n", Tp, Tk, czas_pracy, czas_pracy/60, czas_pracy/3600);

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
    semafor_v(semidF,3); //Podniesienie semaforu, sygnalizuje ze salon jest gotowy, klienci moga zaczac sie pojawiac

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
    sleep(1);
    printf("[F] Wszycy fryzjerzy zakonczyli prace!\n");
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

