#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <unordered_map>
#include <string>
#include <vector>
#include <mpi.h>

class Client {
public:
    Client(int rank, int numProcs);

    int GetMyRank();
private:
    int rank, numProcs;
    std::unordered_map<std::string, MPI_Datatype> Datatypes;

};

#endif  /* __CLIENT_H__ */