#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>
#include <stdbool.h>

#define N 5     //pojemność lotniskowca
#define K 3   //liczba samolotów

pthread_t tid[K];
pthread_mutex_t pas_startowy, zmienna;
pthread_cond_t warunek;

void* samolot(void *arg)
{
    static int v = 0;   //tworzę współdzieloną zmienną lokalną, co jest mniej ryzykowne w porównaniu z implementacją na zmiennych globalnych (ryzyko przypadkowych modyfikacji)
                        //Zmienna ta będzie w odpowiednich wariantach oznaczać pojemność lotniskowca, bądź liczbę samolotów oczekujących na lądowanie.

    //ta pętla to while true
    while(true){
        //  Rozróżniamy w zadaniu dwie sytuacje, 0 -> K>=N oraz 1 -> K < N.
        //  Początkowo lotniskowiec jest pusty i samoloty zaczynają od lądowania.
        
        //Losujemy czas przebywania poza lotniskowcem
        srand(time(NULL) ^ (getpid()<<16));
        int c; c =rand() % 2; sleep(c);

        if (K<N){
            //Na lotniskowcu jest więcej miejsca niż całkowita liczba samolotów. Samoloty lądujące mają priorytet.
            pthread_mutex_lock(&zmienna);
            v++;                  //Samolot lądujący zaznacza, że chce wylądować
            pthread_mutex_unlock(&zmienna);
            pthread_mutex_lock(&pas_startowy); //Próbuje zająć pas startowy
            sleep(1);
            printf("Samolot %d wyladowal\n", pthread_self());
            pthread_mutex_lock(&zmienna);
            v--;    //Samolot zaznacza, że już wylądował
            pthread_cond_signal(&warunek);
            pthread_mutex_unlock(&zmienna);
            pthread_mutex_unlock(&pas_startowy); //Zwalnia pas startowy

        }
        else{
            //Samolotów jest więcej/tyle samo, ile miejsca na lotniskowcu. Samoloty lądujące muszą sprawdzić, czy jest miejsce na lotniskowcu.
            pthread_mutex_lock(&pas_startowy);
            while ( v == N ) {
                printf("Samolot %d czeka na miejsce\n", pthread_self());
                pthread_cond_wait(&warunek, &pas_startowy); //Jeśli lotniskowiec jest pełen (v=N) to czekamy na zmiennej warunkowej.
                printf("Samolot %d ma miejsce\n", pthread_self()); 
            }   
                
            sleep(1);   //Czas potrzebny na lądowanie
            v++;    //Zwiększamy ilość zajętego miejsca na lotniskowcu
            printf("Samolot %d wyladowal\n", pthread_self());
            pthread_mutex_unlock(&pas_startowy);
        }

        //Losujemy czas przebywania na lotniskowcu
        srand(time(NULL) ^ (getpid()<<16));
        int czas; czas =rand() % 2; sleep(czas);

        //Startowanie samolotów
        if (K<N){
            //Na lotniskowcu jest więcej miejsca niż całkowita liczba samolotów. Samoloty lądujące mają priorytet.
            pthread_mutex_lock(&pas_startowy);
            while ( v != 0){
                printf("Samolot %d czeka na ladujace samoloty\n", pthread_self());
                pthread_cond_wait(&warunek, &pas_startowy); //Jeśli są samoloty oczekujące na lądowanie (v!=0) to czekamy na zmiennej warunkowej.
                printf("Droga wolna dla samolotu %d\n", pthread_self()); 
            }
            sleep(1);
            pthread_mutex_unlock(&pas_startowy);

        }
        else{
            pthread_mutex_lock(&pas_startowy);
            if ( v == N ) pthread_cond_signal(&warunek);
            sleep(1);
            v--;
            printf("Samolot %d wystartowal\n", pthread_self());
            pthread_mutex_unlock(&pas_startowy);
        }        

    }

    return NULL;
}

int main(void)
{
    int err;

    if (pthread_mutex_init(&pas_startowy, NULL) != 0)
    {
        printf("\nInicjalizacja zamka\n");
        return 1;
    }
    if (pthread_mutex_init(&zmienna, NULL) != 0)
    {
        printf("\nInicjalizacja zamka\n");
        return 1;
    }
    if (pthread_cond_init(&warunek, NULL) != 0)
    {
        printf("\nInicjalizacja zamka\n");
        return 1;
    }

    //Tworzenie watkow
    for ( int i = 0; i < K; i++)
    {
        err = pthread_create(&(tid[i]), NULL, &samolot, NULL);
        if (err != 0)
            printf("\nTworzenie watku :[%s]", strerror(err));
    }


    //Oczekiwanie na zakończenie watkow przez main
    for ( int i = 0; i < K; i++)
    {
        pthread_join(tid[i], NULL);
    }

    //Sprzatanie
    pthread_mutex_destroy(&pas_startowy);
    pthread_mutex_destroy(&zmienna);
    pthread_cond_destroy(&warunek);

    return 0;
}