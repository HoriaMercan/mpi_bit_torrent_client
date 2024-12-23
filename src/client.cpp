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

Tracker::Tracker(int numProcs):
Entity(0, numProcs) {

}

int Tracker::ReceiveInfoFromClient() {
	MPI_Status status;
	int no_of_files;

	FileHeader header;
	FileHash *hashes;

	MPI_Recv(&no_of_files, 1, MPI_INT, MPI_ANY_SOURCE,
		ControlTag::NoOfFiles, MPI_COMM_WORLD, &status);
	
	int source = status.MPI_SOURCE;

	for (int i = 0; i < no_of_files; i++)
	{
		MPI_Recv(&header, 1, Datatypes["FileHeader"], source,
		ControlTag::InfoTracker, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		hashes = new FileHash[header.segments_no];
		MPI_Recv(hashes, header.segments_no, Datatypes["FileHash"], source,
		ControlTag::InfoTracker, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		std::string filename(header.filename);

		if (file_to_hashes.find(filename) == file_to_hashes.end()) {
			file_to_hashes[filename] = std::make_pair(header.segments_no, hashes);
			file_to_seeds[filename] = std::vector{source};
		} else {
			auto &v = file_to_seeds[filename];
			v.push_back(source);
		}
	}

	char ok = ControlTag::ACK;
	MPI_Send(&ok, 1, MPI_BYTE, source, ControlTag::ACK, MPI_COMM_WORLD);

	std::cout << "Received from " << source << "\n";
	return source;
}