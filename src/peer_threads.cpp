#include "peer_threads.h"

#include <mpi.h>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "control_tags.h"
#include "client.h"

void read_from_input_files(Client *ctx) {
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

	for (const auto &file_name: ctx->wanted_files) {
		auto ans = ctx->RequestFileDetails(file_name);

		int cnt = ans.first;
		FileHash *hashes = ans.second;

		auto new_file = DownloadingFile(file_name, cnt, hashes);
		ctx->HandleDownloadingFile(new_file);

		std::string new_filename = "client" + std::to_string(ctx->GetMyRank()) + "_" + file_name;
		std::ofstream fout(new_filename);
		for (int i = 0; i < cnt; i++) {
			fout << hashes[i] << "\n";
		}

		fout.flush();
		fout.close();
	}

	// Send finished all downloads message
	char info = 0;
	MPI_Send(&info, 1, MPI_CHAR, TRACKER_RANK, ControlTag::FinishedAllDownloads, MPI_COMM_WORLD);
	return NULL;
}

void *upload_thread_func(void *arg)
{
	auto ctx = (Client *) arg;
	
	/* Barrier for marking the end of collecting initial data from peers */
	MPI_Barrier(MPI_COMM_WORLD);
	std::cout << "Finished in " << ctx->GetMyRank() << "\n";
	
	return NULL;
}