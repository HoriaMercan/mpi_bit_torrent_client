#ifndef __BUSYNESS_SCORE__
#define __BUSYNESS_SCORE__

#include <pthread.h>
#include <unordered_map>
#include <queue>

#define DELTA_TIME_LIMIT 10

/**
 * Class for monitoring all the request needed for calculating
 * the busy score.
 * The implementation uses an heuristical approach, in which
 * the busy score will represent the number of other clients
 * making downloading requests in the last DELTA_TIME_LIMIT
 * units of time.
 * 
 * One unit of time will be defined in this context as the time
 * between two consecutive HowBusyReq requests made for a client.
 * (Because they are quite often are pretty constant.)
 * As an improvement, we might use the local system time for
 * synchronisation.
 */
class BusyScore {
public:
    BusyScore();
    ~BusyScore();
    void SubscribeNewRequest(int client, int time);
    int CalculateBusyness(int time);
protected:
    std::unordered_map<int, int> requests_per_client;
    std::queue<std::pair<int, int>> q;
    pthread_mutex_t _mutex;
};

#endif  /* __BUSYNESS_SCORE__ */