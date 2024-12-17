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

int main(){
    char buffer[32];
    pid_t pid_array[F];
    pid_t grupaF, grupaK;
    time_t czas_start;
    int czas_pracy;

    pid_t OriginPID = getpid(); //Pobranie PIDu
    printf("[M] Wlaczono program Menadzer/Kasjer o PID: %d\n", OriginPID);


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

    semafor_p(semid, 9);
    przygotuj_pamiec();
    printf("[M] odczytuje grupy klientow i fryzjerow\n");
    grupaF = shared->pgrp1;
    grupaK = shared->pgrp2;
    shmdt(shared);

    

    printf("[M] Tworze FIFO\n");
    if (mkfifo(fifo_path, 0600) == -1 && errno != EEXIST) {
        perror("[Kierownik] Nie udało się stworzyć FIFO");
        return 1;
    }
    

    //OCZYT PLIKU FIFO
    for (int i = 0; i < F; i++) {
            semafor_v(semid, 2); //Podniesienie semafora dla pliku FIFO dla fryzjerow
            // Otwórz FIFO do odczytu
            fd = open(fifo_path, O_RDONLY);
            if (fd == -1) {
                perror("[M] Nie udało się otworzyć FIFO");
                return 1;
            }

            // Odczytaj dane z FIFO
            ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0'; // Dodaj znak null na końcu ciągu

                // Konwersja odczytanego tekstu na PID
                pid_t pid = (pid_t)atoi(buffer);
                pid_array[i] = pid;
                printf("[M] Odczytano PID od procesu %d: %d\n", i + 1, pid);
                 
            } else if (bytes_read == 0) {
                printf("[M] Proces zapisujący %d zakończył zapis do FIFO.\n", i + 1);
            } else {
                perror("[M] Błąd podczas odczytu z FIFO");
            }

            // Zamknij deskryptor FIFO
            close(fd);

        }

    // Wyświetlenie odczytanych PID-ów
    printf("[M] Odczytane PID-y:\n");
    for (int i = 0; i < F; i++) {
        printf("[M] PID[%d]: %d\n", i, pid_array[i]);
    }
    semafor_v(semid, 4);
    while(semctl(semid, 13, GETVAL)!=K){
        sleep(1);
    }
        
    czas_pracy = (Tk-Tp)*jednostka; //ustala czas pracy
    printf("Fryzjerzy i klienci gotowi! salon zaczynie prace za 3s i bedzie pracowal: %d s!\n", czas_pracy);
    sleep(3);
    czas_start = time(NULL);
    semctl(semid, 14, SETVAL, K);

    while(((time(NULL) - czas_start) < czas_pracy)){
        sleep(1);
    }
        
    printf("[M] Czas pracy salonu dobiega konca, wysylam sygnal! pid grupy fryzjerw: %d, pid grupy klientow: %d\n", grupaF, grupaK);
    kill(-grupaF, SIGUSR1);
    kill(-grupaK, SIGUSR1);
    semafor_p(semid, 6);

    printf("[M] Usuwam pamiec wspoldzielona oraz zbior semaforow!\n");

    if (shmctl(pamiec, IPC_RMID, NULL) == -1) {
        perror("Nie udalo sie usunac pamieci wspoldzielonej");
    }

    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("Nie udalo sie usunac zbioru semaforow");
    }

    printf("[M] Menadzer zakonczyl prace\n");
    
    return 0;

}