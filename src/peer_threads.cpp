#include "peer_threads.h"

#include <mpi.h>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "client.h"

void *download_thread_func(void *arg)
{
	auto ctx = (Client *) arg;
	MPI_Barrier(MPI_COMM_WORLD);
	return NULL;
}

void *upload_thread_func(void *arg)
{
	auto ctx = (Client *) arg;
	const auto filename = "in" + std::to_string(ctx->GetMyRank()) + ".txt";
	std::ifstream fin(filename);

//	std::cout << "Reading from " << filename << ".\n";

	int owned_files, wanted_files;


	return NULL;
}