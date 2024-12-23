#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <unordered_map>
#include <string>
#include <vector>
#include <mpi.h>

#include "file_hash.h"

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

    std::vector<std::pair<FileHeader, FileHash*>> owned_files;
    std::vector<std::string> wanted_files;
    void SendInfoToTracker();
};

class Tracker: public Entity {
public:
    Tracker(int numProcs);

    int ReceiveInfoFromClient();

    std::unordered_map<std::string, std::vector<int>> file_to_seeds;

protected:
    std::unordered_map<std::string, std::pair<int, FileHash *>> file_to_hashes;
    

    
};

#endif /* __CLIENT_H__ */
