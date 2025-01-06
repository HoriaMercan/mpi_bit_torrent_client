#include "busyness_score.h"

BusyScore::BusyScore() {
    pthread_mutex_init(&_mutex, nullptr);
}

BusyScore::~BusyScore() {
    pthread_mutex_destroy(&_mutex);
}

/**
 * Add a new request to the queue.
 */
void BusyScore::SubscribeNewRequest(int client, int time) {

    this->q.push(std::make_pair(client, time));
    if (requests_per_client.find(client) == requests_per_client.end())
        requests_per_client[client] = 0;

    requests_per_client[client]++;
}

/**
 * Calculate the busy score that has to be sent to the clients who
 * request it.
 */
int BusyScore::CalculateBusyness(int time) {
    while (!this->q.empty() && this->q.front().second < time - DELTA_TIME_LIMIT) {
        auto to_delete = this->q.front().first;
        this->q.pop();
        requests_per_client[to_delete]--;
        if(requests_per_client[to_delete] == 0)
            requests_per_client.erase(to_delete);
    }
	
    return requests_per_client.size();
}