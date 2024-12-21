#include "client.h"

Client::Client(int rank, int numProcs):
rank(rank), numProcs(numProcs) {

}

int Client::GetMyRank() { return rank; }