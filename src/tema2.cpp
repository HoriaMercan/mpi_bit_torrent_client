#include <mpi.h>
#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fstream>

#include "peer_threads.h"
#include "client.h"
#include "tracker.h"
#include "file_hash.h"

#define MAX_FILES 10
#define MAX_CHUNKS 100

void tracker(int numtasks, int rank) {
    Tracker me(numtasks);
    me.Datatypes["FileHash"] = SubscribeFileHashTo_MPI();
    me.Datatypes["FileHeader"] = SubscribeFileHeaderTo_MPI();
    
    for (int i = 0; i < numtasks - 1; i++) {
        me.ReceiveInfoFromClient();
    }

    std::cout.flush();
    for (const auto &[k, v]: me.file_to_seeds) {
        std::cout << k << " - ";
        for (const auto &x: v) std::cout << x << " ";
        std::cout << "\n";
    }
    std::cout.flush();

    /* Barrier for marking the end of collecting initial data from peers */
    MPI_Barrier(MPI_COMM_WORLD);

    /* Start to serve requests from the peers/seeds 
        for downloading all the needed files. */
    me.ServeRequests();

    /* Send messages to all the clients to stop (by stoping their
        upload thread). */
    me.StopUploadingClients();
   
}

void peer(int numtasks, int rank) {
    pthread_t download_thread;
    pthread_t upload_thread;
    void *status;
    int r;

    Client me(rank, numtasks);
    me.Datatypes["FileHash"] = SubscribeFileHashTo_MPI();
    me.Datatypes["FileHeader"] = SubscribeFileHeaderTo_MPI();

    r = pthread_create(&download_thread, NULL, download_thread_func, (void *) &me);
    if (r) {
        printf("Eroare la crearea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_create(&upload_thread, NULL, upload_thread_func, (void *) &me);
    if (r) {
        printf("Eroare la crearea thread-ului de upload\n");
        exit(-1);
    }

    r = pthread_join(download_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_join(upload_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de upload\n");
        exit(-1);
    }
}
 
int main (int argc, char *argv[]) {
    int numtasks, rank;
 
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI nu are suport pentru multi-threading\n");
        exit(-1);
    }
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == TRACKER_RANK) {
        tracker(numtasks, rank);
    } else {
        peer(numtasks, rank);
    }

    MPI_Finalize();
}
