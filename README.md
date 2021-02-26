# Concurrent-computing

Lotniskowiec
Na lotniskowcu ląduje i startuje K samolotów. W tym celu potrzebują wyłącznego dostępu do pasa. Lotniskowiec może pomieścić pewną ustaloną liczbę N samolotów. 
Jeśli K < N, wówczas priorytet w dostępie do pasa mają samoloty lądujące.
Cel zadania: synchronizacja samolotów (pas i miejsce na lotnisku to zasoby).

Czytelnicy i pisarze (ReadersWriters)
Wersja problemu czytelników i pisarzy, gdzie:
1. jest ustalona liczba procesów N;
2. każdy proces działa naprzemiennie w dwóch fazach: fazie relaksu i fazie korzystania z czytelni;
3. w fazie relacji proces może (choć nie musi) zmienić swoją rolę; z pisarza na czytelnika lub z czytelnika na pisarza;
4. przechodząc do fazy korzystania z czytelni proces musi uzyskać dostęp we właściwym dla sowjej aktualnej roli trybie;
5. pisarz umieszcza efekt swojej pracy - swoje dzieło - w formie komunikaty w kolejce komunikatów, gdzie komunikat ten pozostaje do momentu, aż odczytają go wszytkie procesy, które w momencie wydania dzieła czekały na dostęp do czytelni (po odczytaniu przez wymaganą liczbę procesów komunikat jest usuwany);
6. pojemność kolejki komunikatów - reprezentującej półkę z książkami - jest ograniczona, tzn. nie może ona przechowywać więcej niż K dzieł;
