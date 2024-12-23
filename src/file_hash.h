#ifndef __FILE_HASH_H__
#define __FILE_HASH_H__

#include <string>
#include <iostream>
#include <mpi.h>

#define HASH_SIZE 32
#define MAX_FILENAME 15

struct FileHeader
{
    char filename[MAX_FILENAME];
    int segments_no;

    FileHeader();
    FileHeader(const std::string &filename_, int segments_no_);
};

MPI_Datatype SubscribeFileHeaderTo_MPI();

struct FileHash
{
    char x[HASH_SIZE];

    FileHash();
    FileHash(const std::string &s);
    friend std::ostream &operator<<(std::ostream &o, const FileHash &f);
};

MPI_Datatype SubscribeFileHashTo_MPI();

#endif /* __FILE_HASH_H__ */