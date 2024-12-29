#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <mpi.h>

#include <semaphore.h>
#include <pthread.h>

#include "file_hash.h"

#define TRACKER_RANK 0

class Entity {
public:
    Entity(int rank, int numProcs);
    ~Entity();
    int GetMyRank();

    std::unordered_map<std::string, MPI_Datatype> Datatypes;

protected:
    int rank, numProcs;
};

class Client: public Entity {
public:
    Client(int rank, int numProcs);
    ~Client();

    std::vector<std::pair<FileHeader, FileHash*>> owned_files;

    std::vector<std::string> wanted_files;
    void SendInfoToTracker();

    void HandleDownloadingFile(DownloadingFile &file);

    std::pair<int, FileHash *> RequestFileDetails(const std::string &filename);
    void SendMessageForFileDownloaded(const std::string &filename);

    bool CheckExistingSegment(const char *filename, const FileHash& data);
    bool GetFileWithGivenHash(int client, const FileHash &hash);
    pthread_barrier_t barrier;
private:
    DownloadingFile *current_downloading;
    pthread_mutex_t downloading_mutex;
    
};

#endif /* __CLIENT_H__ */
