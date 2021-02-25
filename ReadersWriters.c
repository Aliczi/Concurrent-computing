#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <sys/types.h>   
#include <sys/ipc.h>    
#include <sys/sem.h>     
#include <sys/shm.h>
#include <sys/msg.h>


/* Alicja Miłoszewska 145213 */

#define N 5
#define NIEZDECYDOWANY 10 //określa ile razy proces może zmienić decyzję w trakcie trwania fazy relaksu


static struct sembuf buf;

void zwolnij(int semid, int semnum)
{
    buf.sem_num = semnum;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    if (semop(semid, &buf, 1) == -1)
    {
        perror("Podnoszenie semafora");
        exit(1);
    }
}

void zajmij(int semid, int semnum)
{
    buf.sem_num = semnum;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    if (semop(semid, &buf, 1) == -1)
    {
        perror("Opuszczenie semafora");
        exit(1);
    }
}

struct buf_elem
{
    long mtype;
    int mvalue;
};

#define PUSTY 1
#define PELNY 2

int main()
{
    int id_sem; //id tablicy semaforów
    int key = IPC_PRIVATE; //dla wygody testowania
    int *liczba_czytelnikow;
    int id_shm; //id pamięci współdzielonej
    int id_msg; //id kolejki komunikatów
    int K=3*sizeof(struct buf_elem);        //Liczba książek na półce
    struct buf_elem elem;
    struct msqid_ds ss;
    ss.msg_qbytes = K;

    //Tworzenie tablicy z dwoma semaforami
    id_sem = semget(key, 2, IPC_CREAT|0600); 
    if (id_sem == -1)
    {
        perror("Utworzenie tablicy semaforów"); exit(1);
    }

    //Nadajemy semaforowi '0' wartość początkową 1. 
    //Zakładam, że będzie on odpowiedzialny za blokowanie pisarzowi dostępu do czytelni,
    //jeśli nie będzie ona pusta. 
    if ( semctl( id_sem, 0, SETVAL, (int)1) == -1 )
    {                                    
        perror("Nadanie wartości semaforowi 0"); exit(1); 
    }

    //Drugi semafor w tablicy będzie "pilnował" zmiennej liczba_czytelników,
    //aby nie modyfikowały jej dwa procesy na raz.
    if ( semctl( id_sem, 1, SETVAL, (int)1) == -1)
    {
        perror("Nadanie wartości semaforowi 1"); exit(1);
    }

    //Tworzymy fragment pamięci współdzielonej
    id_shm = shmget(key, sizeof(int), 0600);
    liczba_czytelnikow = (int*)shmat(id_shm, NULL, 0);

    //Tworzymy kolejkę komunikatów
    id_msg = msgget(key, IPC_CREAT|IPC_EXCL|0600);
    msgctl(id_msg, IPC_SET, &ss);

    if (id_msg == -1){
        id_msg = msgget(key, IPC_CREAT|0600);
    }
    else{
        elem.mtype = PUSTY;
        for (int f=0; f<K; f++){
            msgsnd(id_msg, &elem, sizeof(elem.mvalue), 0);
        }
    }



    for (int i=0; i<N; i++)
    {
       
       if ( fork() == 0 )
       {
            liczba_czytelnikow = (int*)shmat(id_shm, NULL, 0);
            srand(time(NULL) ^ (getpid()<<16));
            //Losowanie roli 0-pisarz, 1-czytelnik
            int rola;
            rola =rand() % 2;

            while(true)
            {
                    //czas trwania fazy relaksu
                double czas_relaksu;
                czas_relaksu=rand() % 3 + 1;

                //Zmiana roli procesu w trakcie fazy relaksu
                bool zmiana=true; //ogranicza ilość zmian roli w trakcie trwania fazy relaksu do jednej
                for (int j=0; j < NIEZDECYDOWANY; j++){
                    if (zmiana){
                        int nowa_rola=rand() % 2;
                        if (nowa_rola!=rola) { rola= nowa_rola; zmiana=false;}
                    }
                    double czas_spania=czas_relaksu/NIEZDECYDOWANY *1000000;
                    usleep(czas_spania);
                }

                //przechodzimy do fazy korzystania z czytelni

                //Rola 0 = pisarz. Tylko jeden może być na raz w czytelni.
                //Nie może być żadnych innych czytelników.
                if(rola == 0) 
                {
                    zajmij(id_sem, 0);
                    
                    /* __________ Operacje na kolejce komunikatów___________*/
                    msgrcv(id_msg, &elem, sizeof(elem.mvalue), PUSTY,  IPC_NOWAIT);  //flaga IPC_NOWAIT nie dopuszcza do zakleszczenia z powodu braku "miejsca na półce"
                    elem.mvalue = *liczba_czytelnikow;                         
                    elem.mtype = PELNY;
                    msgsnd(id_msg, &elem, sizeof(elem.mvalue), IPC_NOWAIT);
                    /* ________________Koniec tych operacji__________________*/

                    printf("Jestem pisarzem ~ %d\n", getpid());
                    zwolnij(id_sem, 0);
                }
                else    //Rola 1 = czytelnik. Dowolna liczba czytelników w czytelni.
                {
                    zajmij(id_sem, 1);   //Zajmujemy semafor od zmiennej liczba_czytelnikow
                    *liczba_czytelnikow++;
                    if (*liczba_czytelnikow == 1)    //Sprawdzamy czy czytelnik nie jest pierwszym czytelnikiem w czytelni.
                    {
                        zajmij(id_sem, 0); //Czytelnik próbuje zablokować pisarza.
                    }
                    zwolnij(id_sem, 1); //Oddajemy semafor '1' innym czytelnikom

                    /* __________ Operacje na kolejce komunikatów___________*/
                    msgrcv(id_msg, &elem, sizeof(elem.mvalue), PELNY, IPC_NOWAIT);
                    if ( elem.mvalue == 1 )
                    {
                        elem.mtype = PUSTY;
                        msgsnd(id_msg, &elem, sizeof(elem.mvalue), IPC_NOWAIT);
                    }
                    else
                    {
                        elem.mtype--;
                        msgsnd(id_msg, &elem, sizeof(elem.mvalue), IPC_NOWAIT);
                    } 
                    
                    printf("Jestem czytelnikiem ~ %d\n", getpid()); 


                    /* ________________Koniec tych operacji__________________*/

                    zajmij(id_sem, 1);  //Przy wychodzeniu znowu zajmujemy semafor '1'
                    *liczba_czytelnikow--;   //Dekrementujemy liczbę czytelników w czytelni
                    if (*liczba_czytelnikow == 0) //oraz sprawdzamy czy nie jesteśmy ostatnim czytelnikiem wychodzącym z czytelni.
                    {
                        zwolnij(id_sem, 0); //jeśli tak, to zwalniamy semafor '0'
                    }
                    zwolnij(id_sem, 1);  //Zwalniamy semafor '1'
                }
            }
            
            exit(0);

       }
    }
    getchar();
    semctl(id_sem, IPC_RMID, 0);
    shmctl(id_shm, IPC_RMID, 0);
    exit(0);
}