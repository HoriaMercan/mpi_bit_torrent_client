#ifndef __BUSYNESS_SCORE__
#define __BUSYNESS_SCORE__

#include <pthread.h>
#include <unordered_map>
#include <queue>

#define REQUESTS_LIMIT 10

class BusyScore {
public:
    BusyScore();
    ~BusyScore();
    void SubscribeNewRequest(int client);
    int CalculateBusyness();
protected:
    std::unordered_map<int, int> requests_per_client;
    std::queue<int> q;
    pthread_mutex_t _mutex;
};

#endif  /* __BUSYNESS_SCORE__ */