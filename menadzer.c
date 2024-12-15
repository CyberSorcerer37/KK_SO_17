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
    pid_t grupaF;

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

    semafor_p(semid, 5);
    przygotuj_pamiec();
    printf("[M] odczytuje grupy klientow i fryzjerow\n");
    grupaF = shared->pgrp1;
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
        printf("[M] Wysylam sygnal do fryzjerow ze koniec pracy, pid grupy: %d\n", grupaF);
        kill(-grupaF, SIGUSR1);
        semafor_p(semid, 6);
        printf("[M] Menadzer zakonczyl prace\n");
    
    return 0;

}