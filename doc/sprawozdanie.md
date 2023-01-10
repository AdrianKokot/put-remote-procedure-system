# System zdalnego wywoływania procedur
Wykonanie: Tomasz Cichy (145113), Adrian Kokot (148165)

## Opis protokołu komunikacyjnego

Serwer i klient komunikują się za pomocą wiadomości w formacie:

```
wielkość_wiadomości\n
wiadomość
```

Tekst aż do znaku nowej linii oznacza ile bitów zajmuje wiadomość, a następnie jest przekazana ta wiadomość.

Serwer jest przygotowany na dwa typy wiadomości:

- `GET_SCRIPTS` - serwer zwraca listę dostępnych procedur (każda w nowej linii)
- `EXEC script.py arg1 arg2 ... argn` - serwer zwraca wynik wykonania procedury o nazwie `script.py` z wykorzystaniem podanych argumentów

W przypadku nie znalezienia podanej procedury lub podaniu nieoczekiwanej komendy, serwer odpowie wiadomością z informacją o błędzie.


## Opis implementacji

### Serwer

Plik `./server/server.c` zawiera implementację serwera w języku c. Serwer jest współbieżny, do czego wykorzystano deskryptory (funkcja `select`).

W celu uzyskania listy dostępnych proecdur, serwer wykonuje funkcję systemową `ls -1`. W celu wykonania procedur serwer uruchamia plik za pomocą interpretera (polecenie `python3`).

## Sposób kompilacji

### Serwer

```bash
gcc -Wall ./server/server.c -o ./server.out
```

### Klient

## Wymagania

Serwer dla poprawnego działania potrzebuje mieć w ścieżce dostęp do interpretera języka python za pomocą polecenia `python3`.