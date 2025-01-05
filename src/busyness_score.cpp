#include "busyness_score.h"

BusyScore::BusyScore() {
    pthread_mutex_init(&_mutex, nullptr);
}

BusyScore::~BusyScore() {
    pthread_mutex_destroy(&_mutex);
}

void BusyScore::SubscribeNewRequest(int client) {
    if (this->q.size() == REQUESTS_LIMIT) {
        int to_delete = this->q.front();
        requests_per_client[to_delete]--;
        if (requests_per_client[to_delete] == 0 && to_delete != client) {
            requests_per_client.erase(to_delete);
        }
    }

    this->q.push(client);
    if (requests_per_client.find(client) == requests_per_client.end())
        requests_per_client[client] = 0;

    requests_per_client[client]++;
}

int BusyScore::CalculateBusyness() {
	if (requests_per_client.find(0) == requests_per_client.end())
		return requests_per_client.size();
	return requests_per_client.size() - 1;
}