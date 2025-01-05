#include "peer_threads.h"

#include <mpi.h>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "control_tags.h"
#include "client.h"

void read_from_input_files(Client *ctx)
{
	const auto filename = "in" + std::to_string(ctx->GetMyRank()) + ".txt";
	std::ifstream fin(filename);

	int owned_files, wanted_files, file_segments_no;
	std::string file_name, hash_;

	fin >> owned_files;
	for (int i = 0; i < owned_files; i++) {
		fin >> file_name >> file_segments_no;
		FileHeader header(file_name, file_segments_no);
		FileHash *v = new FileHash[file_segments_no];
		for (int j = 0; j < file_segments_no; j++) {
			fin >> hash_;
			v[j] = FileHash(hash_);
		}

		ctx->owned_files.emplace_back(header, v);
	}

	fin >> wanted_files;
	for (int i = 0; i < wanted_files; i++) {
		fin >> file_name;
		ctx->wanted_files.push_back(file_name);
	}
}

void *download_thread_func(void *arg)
{
	auto ctx = (Client *) arg;
	read_from_input_files(ctx);

	ctx->SendInfoToTracker();
	pthread_barrier_wait(&ctx->barrier);

	for (const auto &file_name: ctx->wanted_files) {
		auto ans = ctx->RequestFileDetails(file_name);

		int cnt = ans.first;
		FileHash *hashes = ans.second;

		auto new_file = DownloadingFile(file_name, cnt, hashes);
		ctx->HandleDownloadingFile(new_file);

		ctx->SendMessageForFileDownloaded(file_name);

		std::string new_filename =
		"client" + std::to_string(ctx->GetMyRank()) + "_" + file_name;
		std::ofstream fout(new_filename);
		for (int i = 0; i < cnt; i++) {
			fout << hashes[i] << "\n";
		}

		fout.flush();
		fout.close();
	}

	// Send finished all downloads message
	char info = 0;
	MPI_Send(&info, 1, MPI_CHAR, TRACKER_RANK,
			 ControlTag::FinishedAllDownloads, MPI_COMM_WORLD);
	return NULL;
}

void *upload_thread_func(void *arg)
{
	auto ctx = (Client *) arg;

	/* Barrier for marking the end of collecting initial data from peers */
	MPI_Barrier(MPI_COMM_WORLD);
	pthread_barrier_wait(&ctx->barrier);
	MPI_Status status;

	char filename_[MAX_FILENAME];
	FileHash data;
	int source;
	char ans = 0;

	// Loop until we will receive the STOP message from the tracker.
	while (true) {
		MPI_Recv(&filename_, MAX_FILENAME, MPI_CHAR, MPI_ANY_SOURCE,
				 ReqFile, MPI_COMM_WORLD, &status);

		if (status.MPI_SOURCE == TRACKER_RANK)
			break;

		source = status.MPI_SOURCE;

		MPI_Recv(&data, 1, ctx->Datatypes["FileHash"], source,
				 ReqFile, MPI_COMM_WORLD, &status);

		if (ctx->CheckExistingSegment(filename_, data)) {
			// Send back the file (in our case, ACK)
			ans = ControlTag::ACK;
			ctx->busy_score.SubscribeNewRequest(source);
		} else {
			ans = ControlTag::NACK;
			ctx->busy_score.SubscribeNewRequest(0);
		}

		MPI_Send(&ans, 1, MPI_CHAR, source, AnsFile, MPI_COMM_WORLD);
	}

	return NULL;
}

void *get_loading_info_thread_func(void *arg)
{
	auto ctx = (Client *) arg;

	MPI_Status status;
	char _req;
	int source;

	pthread_barrier_wait(&ctx->barrier);

	while (true) {
		MPI_Recv(&_req, 1, MPI_CHAR, MPI_ANY_SOURCE,
				 ControlTag::HowBusyReq, MPI_COMM_WORLD, &status);

		if (status.MPI_SOURCE == TRACKER_RANK) {
			break;
		}

		source = status.MPI_SOURCE;
		_req = ctx->busy_score.CalculateBusyness();

		MPI_Send(&_req, 1, MPI_CHAR, source,
				 ControlTag::HowBusyAns, MPI_COMM_WORLD);
	}

	return nullptr;
}