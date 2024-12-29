#include "client.h"
#include "control_tags.h"

Entity::Entity(int rank, int numProcs):
rank(rank), numProcs(numProcs) {

}


Entity::~Entity() {
	for (auto &x: this->Datatypes) {
		auto &datatype = x.second;
		MPI_Type_free(&datatype);
	}
}


int Entity::GetMyRank() { return rank; }

Client::Client(int rank, int numProcs):
Entity(rank, numProcs) {
	pthread_mutex_init(&this->downloading_mutex, nullptr);
	current_downloading = nullptr;
}

Client::~Client() {
	pthread_mutex_destroy(&this->downloading_mutex);
}

void Client::SendInfoToTracker() {
	int no_of_files = this->owned_files.size();
	MPI_Send(&no_of_files, 1, MPI_INT, 0, ControlTag::NoOfFiles, MPI_COMM_WORLD);
	for (const auto &x: this->owned_files) {
		auto &header = x.first;
		auto &hash = x.second;

		MPI_Send(&header, 1, this->Datatypes["FileHeader"], 0, ControlTag::InfoTracker, MPI_COMM_WORLD);
		MPI_Send(hash, header.segments_no, this->Datatypes["FileHash"], 0, ControlTag::InfoTracker, MPI_COMM_WORLD);
	}

	char ok;
	
	MPI_Recv(&ok, 1, MPI_BYTE, 0, ControlTag::ACK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

void Client::HandleDownloadingFile(DownloadingFile &file) {
	pthread_mutex_lock(&downloading_mutex);
	current_downloading = &file;
	pthread_mutex_unlock(&downloading_mutex);

	int step = 0;
	int segments_to_receive = file.header.segments_no;

	// Array for mantaining the peers/seeds for downloading segments
	int *peers = new int[numProcs - 1]();
	int peers_cnt = 0;

	MPI_Status status;

	while (segments_to_receive > 0) {
		if (step % 10 == 0) {
			// Make a request to the tracker to get info about
			// other peers/seeds that have the same file
			char * char_vec_filename = file.header.filename;
			int char_vec_size = strnlen(char_vec_filename, MAX_FILENAME) + 1;
			
			std::cout << "Want who has this file for : " << char_vec_filename << " with size " << char_vec_size << std::endl;

			MPI_Send(char_vec_filename, char_vec_size, MPI_CHAR, TRACKER_RANK, ControlTag::WhoHasThisFile, MPI_COMM_WORLD);
			MPI_Recv(peers, numProcs, MPI_INT, TRACKER_RANK, ControlTag::WhoHasThisFile, MPI_COMM_WORLD, &status);

			MPI_Get_count(&status, MPI_INT, &peers_cnt);
		
			std::cout << "WhoHasThisFile no " << step / 10 << " in " << rank << " :";
			for (int i = 0; i < peers_cnt; i++) {std::cout << peers[i] << "\t";}
			std::cout << "\n";
		}

		segments_to_receive--;
		step++;
	}


	delete peers;
	pthread_mutex_lock(&downloading_mutex);

	this->owned_files.push_back(file.ConvertToDownloaded());
	current_downloading = nullptr;

	pthread_mutex_unlock(&downloading_mutex);
}

std::pair<int, FileHash *> Client::RequestFileDetails(const std::string &filename) {
	int cnt = 0;
	int bytes_cnt = filename.size() + 1;
	MPI_Status status;

	std::cout << "Request " << filename << " in client " << rank << "\n";

	MPI_Send(filename.c_str(), bytes_cnt, MPI_CHAR, TRACKER_RANK, ControlTag::GiveMeHashes, MPI_COMM_WORLD);

	MPI_Recv(&cnt, 1, MPI_INT, TRACKER_RANK, ControlTag::GiveMeHashes, MPI_COMM_WORLD, &status);

	FileHash *hashes = new FileHash[cnt];
	MPI_Recv(hashes, cnt, Datatypes["FileHash"], TRACKER_RANK, ControlTag::GiveMeHashes, MPI_COMM_WORLD, &status);

	return std::make_pair(cnt, hashes);
}
