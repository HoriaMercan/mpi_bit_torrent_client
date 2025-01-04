#include "tracker.h"
#include "control_tags.h"

Tracker::Tracker(int numProcs):
Entity(0, numProcs) {

}

Tracker::~Tracker() {
    for (auto &[__, pairs]: file_to_hashes) {
        auto &[_, array] = pairs;
        delete []array;
    }
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

		std::cout << "received " << filename << " from " << source << "\n";
	}

	char ok = ControlTag::ACK;
	MPI_Send(&ok, 1, MPI_BYTE, source, ControlTag::ACK, MPI_COMM_WORLD);

	return source;
}

void Tracker::ServeRequests() {
	int remaining_peers_for_download = this->numProcs - 1;
	char data[MAX_FILENAME];
	MPI_Status status;
	int bytes_cnt;

	ControlTag tag = ControlTag::None;
	int source;

	while (remaining_peers_for_download > 0) {
		MPI_Recv(data, MAX_FILENAME, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG,
			MPI_COMM_WORLD, &status);

		tag = static_cast<ControlTag>(status.MPI_TAG);
		source = status.MPI_SOURCE;
		MPI_Get_count(&status, MPI_CHAR, &bytes_cnt);

		// std::cout << "Received " << bytes_cnt << " from " << source << " with TAG: " << tag << "\n";

		switch (tag) {
			case FinishedAllDownloads:
				remaining_peers_for_download--;
				break;
			
			case WhoHasThisFile: 
				{
					std::string filename(data);

					std::cout.flush();
					std::cout << "WhoHasThisFile from " << source << "with: " << filename << "\n";
                    std::cout.flush();

					auto &all_seeds = this->file_to_seeds[filename];

					std::vector<int> vector_to_be_sent(all_seeds.begin(), all_seeds.end());

					if (this->file_to_peers.find(filename) != this->file_to_peers.end()) {
						vector_to_be_sent.insert(vector_to_be_sent.begin(),
						file_to_peers[filename].begin(), file_to_peers[filename].end());
					}

                    // Do not send the source as an answer even though it might be already a peer
                    std::vector<int>::iterator it;
                    for (it = vector_to_be_sent.begin(); it != vector_to_be_sent.end(); it++) {
                        if (*it == source) {
                            it = vector_to_be_sent.erase(it);
                            break;
                        }
                    }

					int size_ = vector_to_be_sent.size();
					MPI_Send(&vector_to_be_sent[0], size_, MPI_INT, source, tag, MPI_COMM_WORLD);


				}
				break;
			case GiveMeHashes:
				{
					std::string filename(data);

					// std::cout.flush();
					// std::cout << "GiveMeHashes from " << source << " with:" << filename << "\n";

					auto &vec = this->file_to_hashes[filename];

					auto &vec_peers = this->file_to_peers[filename];
					vec_peers.insert(source);
					MPI_Send(&vec.first, 1, MPI_INT, source, tag, MPI_COMM_WORLD);
					MPI_Send(vec.second, vec.first, Datatypes["FileHash"], source, tag, MPI_COMM_WORLD);
				}
				break;
            case FinishedFileDownload:
                {
                    std::string filename(data);
                    auto &vec_peers = this->file_to_peers[filename];

                    /* Search for this peer in the peer list and erase it */
                    vec_peers.erase(source);

                    this->file_to_seeds[filename].push_back(source);
                }
                break;
			default: /* Should never get here! */
			;
		}


	}
}

void Tracker::StopUploadingClients() {
    char empty = 0;
    for (int i = 1; i < numProcs; i++) {
        MPI_Send(&empty, 1, MPI_CHAR, i, ControlTag::ReqFile, MPI_COMM_WORLD);
		MPI_Send(&empty, 1, MPI_CHAR, i, ControlTag::HowBusyReq, MPI_COMM_WORLD);
    }
}