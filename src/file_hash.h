#ifndef __FILE_HASH_H__
#define __FILE_HASH_H__

#include <string>
#include <iostream>
#include <mpi.h>

#define HASH_SIZE 32
#define MAX_FILENAME 15

/**
 * Struct for a better organization of files metadata
 *  -> filename
 *  -> segments number
 */
struct FileHeader
{
    char filename[MAX_FILENAME];
    int segments_no;

    FileHeader();
    FileHeader(const std::string &filename_, int segments_no_);
    bool MatchesFilename(const char *c);
};

MPI_Datatype SubscribeFileHeaderTo_MPI();

/**
 * Struct for organizing a hash and overloading operators for
 * a better use.
 */
struct FileHash
{
    char x[HASH_SIZE];

    FileHash();
    FileHash(const std::string &s);
    friend std::ostream &operator<<(std::ostream &o, const FileHash &f);
    bool operator==(const FileHash &other);
};

MPI_Datatype SubscribeFileHashTo_MPI();

/**
 * Struct for organizing a file details of a current
 * downloading file.
 * 
 * downloaded array will contain the following info:
 *  downloaded[i] == true <=> the segment with hash hashes[i]
 *          has been downloaded.
 */
struct DownloadingFile
{
    FileHeader header;
    FileHash *hashes;
    bool *downloaded;

    DownloadingFile(std::string filename, int segments_no, FileHash *hashes);
    std::pair<FileHeader, FileHash *> ConvertToDownloaded();
};

#endif /* __FILE_HASH_H__ */