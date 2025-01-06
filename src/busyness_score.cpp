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
    pthread_mutex_lock(&_mutex);
    this->q.push(std::make_pair(client, time));
    if (requests_per_client.find(client) == requests_per_client.end())
        requests_per_client[client] = 0;

    requests_per_client[client]++;
    pthread_mutex_unlock(&_mutex);
}

/**
 * Calculate the busy score that has to be sent to the clients who
 * request it.
 */
int BusyScore::CalculateBusyness(int time) {
    int ans;

    pthread_mutex_lock(&_mutex);
    while (!this->q.empty() && this->q.front().second < time - DELTA_TIME_LIMIT) {
        auto to_delete = this->q.front().first;
        this->q.pop();
        requests_per_client[to_delete]--;
        if(requests_per_client[to_delete] == 0)
            requests_per_client.erase(to_delete);
    }
    ans = requests_per_client.size();
    pthread_mutex_unlock(&_mutex);
	
    return ans;
}