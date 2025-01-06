#ifndef __TRACKER_H__
#define __TRACKER_H__

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <mpi.h>

#include "file_hash.h"
#include "client.h"

/**
 * Class for important functionalities
 * of the tracker.
 */
class Tracker: public Entity {
public:
    Tracker(int numProcs);
    ~Tracker();

    int ReceiveInfoFromClient();

    std::unordered_map<std::string, std::vector<int>> file_to_seeds;
    std::unordered_map<std::string, std::unordered_set<int>> file_to_peers;

    void ServeRequests();

    void StopUploadingClients();

protected:
    std::unordered_map<std::string, std::pair<int, FileHash *>> file_to_hashes;
    
};



#endif  /* __TRACKER_H__ */