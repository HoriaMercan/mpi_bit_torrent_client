# Tema Algoritmi Paraleli si Distribuiti - BitTorrent Protocol
## Horia Mercan - grupa 333CA

In cadrul acestui README voi detalia implementaria protocolului BitTorrent pentru partajarea unor fisiere.

## Organizarea componentelor protocolului

### Tracker

Principalele functionalitati ale trackerului sunt implementate in fisierele tracker.h & tracker.cpp. Structurile de date principale sunt:
```C++
map<string, pair<int, FileHash*>> file_to_hashes;  // Fișier -> (nr_segmente, hash-uri)
map<string, vector<int>> file_to_seeds;            // Fișier -> lista de seeds
map<string, set<int>> file_to_peers;               // Fișier -> lista de peers
```

Functionalitatile cheie ale trackerului sunt:
```C++
ReceiveInfoFromClient(); // Primește informații inițiale despre fișierele deținute
ServeRequests(); // Procesează cererile de la peers
StopUploadingClients(); // Oprește clienții la finalul transferurilor
```

In cadrul functiei ```ServeRequests()```, trackerul primeste requesturi de la clienti (legate de cine detine un anumit fisier sau ce hashuri sunt
asociate unui fisier) si totodata primeste mesaje despre starea unui peers/seed in raport cu procesul de download (primeste informatii despre
finalizarea tuturor descarcarilor/ descarcarii unui fisier).


### Clients (Peers/Seeds)

Logica clientilor este implementata in urmatoarele fisiere: peer_threads.cpp si client.cpp. Fiecare nod ruleaza pe 3 threaduri:
* Thread de Download (download_thread_func)
    * se ocupa cu descarcarea succesiva a fisierelor de la alti clienti
* Thread de Upload (upload_thread_func)
    * proceseaza cereri de la alti peers
    * verifica existenta in storage-ul local al unui segment
* Thread de Informare (get_loading_info_thread_func)
    * calculeaza si raspunde la cereri cu privire la gradul de incarcare
    * foloseste un scor de incarcare

#### Descarcarea de la alti seeds/peers

In cadrul functiei ```Client::HandleDownloadingFile(DownloadingFile &)``` se vor gasi doua implementari de algoritmi pentru descarcarea segmentelor unui fisier: round robin si un algoritm bazat pe o euristica data de gradul de incarcare al clientilor (default in Makefile-ul atasat este folosit cel bazat pe gradul de incarcare: BUSY_SCORE).

Pentru a decide de la ce client facem request-ul pentru a descarca un fisier, trimitem o cerere catre toti clientii pentru a ne transmite gradul lor de busyness (acest request este prelucrat mai rapid, de catre un thread dedicat; in conditii realea acest request va fi executat mult mai rapid decat descarcarea propriu-zisa).

Pentru a detalia ce inseamna gradul/scorul de busyness (clasa <b>BusyScore</b> din <b>busyness_score.h</b>):
scorul = nr. de clienti ale caror requesturi de descarcare au fost servite in ultimele 10 unitati de timp. Astfel, daca un client are un scor cat mai mic, inseamna ca a servit putine requesturi catre alti clienti (deci ar putea sa primeasca alte requesturi de la alti clienti ce vor un anumit fisier).

Dupa ce obtinem scorul fiecarui client, alegem clientul cu scorul cel mai mic (acesta este cel mai putin ocupat) si incercam sa descarcam de la acesta segmentul dorit.

```C++
// Pentru un singur segment:

// Obtinem scorurile pentru ceilalti peers/seeds
auto scores_and_peers = GetScoresForPeers(peers, peers_cnt); // // [{scor, peerID/seedID}]

std::set < std::pair < char, int >> ::iterator
it = scores_and_peers.begin();

do {
    if (it == scores_and_peers.end())
        it = scores_and_peers.begin();

    // Incercam sa obtinem segmentul de la cel mai optim peer/seed
    if (GetFileWithGivenHash(it->second, <hash-ul segmentului>)) {
        // Marcam fisierul ca fiind descarcat (echivalentul cu a descarca un fisier)
        downloaded[<indicele segmentului curent>] = true;
        break;
    } else {
        it = std::next(it);
    }
} while (true);
```

Observatie. Implementarea round-robin nu tine cont de scoruri, asa ca peers sunt parcursi intr-o ordine fixa.

#### Upload-ul fisierelor pentru alti peers
Bazat pe hash-ul primit si numele fisierului dorit, un client cauta daca are acest in local storage si trimite raspuns ACK/NACK.

### Structuri de date folosite

#### ControlTag (control_tags.h)
Aceasta este o enumeratie pentru deosebirea tagurilor folosite intre clienti si tracker. Astfel, mesajele pot si separate si directionate pe threadurile pe care sunt asteptate, fara a mai fi nevoie de simularea stabilirii conexiunii pe un port separat (ca in cazul TCP).

#### FileHash (file_hash.h)
Folosita pentru stocarea unui hash. Va avea suport in MPI pentru handling mai usor al mesajelor.

#### FileHeader (file_hash.h)
Folosita pentru stocarea metadatelor unui fisier: nume si numar de segmente.

## Observatii generale
Am observat in mod empiric ca euristica bazata pe scorul de incarcare al clientilor duce la rezultate mai bune in ceea ce priveste:
* Distribuirea sarcinii de download catre alti clienti: dispersia numarului de segmente descarcate de la fiecare client scade fata de un algoritm bazat doar pe o tehnica round-robin. (Segmentele incep sa fie preluate in numar mai mare si de la alti peers)
* Numarul de segmente pentru care s-a dat request dar acestea nu au fost gasite scade (=> numarul de requesturi redundante) fata de round-robin.

Pentru testarea cu algoritmul bazat pe busyness score, vom folosi in Makefile urmatorul flag:
```bash
LINKFLAGS=BUSY_SCORE
```
Pentru testarea cu algoritmul bazat pe round robin, vom folosi in Makefile urmatorul flag:
```bash
LINKFLAGS=ROUND_ROBIN
```

Pentru afisarea unor metrici si a unor detalii de debugging, vom folosi flagurile:
```bash
LINKFLAGS=BUSY_SCORE DEBUG
```