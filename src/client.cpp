#include "client.h"
#include "control_tags.h"

Entity::Entity(int rank, int numProcs) : rank(rank), numProcs(numProcs)
{
}

Entity::~Entity()
{
	for (auto &x: this->Datatypes) {
		auto &datatype = x.second;
		MPI_Type_free(&datatype);
	}
}

int Entity::GetMyRank() { return rank; }

Client::Client(int rank, int numProcs) : Entity(rank, numProcs)
{
	pthread_mutex_init(&this->downloading_mutex, nullptr);
	current_downloading = nullptr;
	pthread_barrier_init(&this->barrier, nullptr, 3);
	get_info_counter = 0;
}

Client::~Client()
{
	pthread_mutex_destroy(&this->downloading_mutex);
	pthread_barrier_destroy(&this->barrier);

	for (auto &[_, v]: owned_files) {
		delete[] v;
	}
}

/**
 * This function is used to send to the tracker initial information about
 * all the files that the client owns.
 */
void Client::SendInfoToTracker()
{
	int no_of_files = this->owned_files.size();
	MPI_Send(&no_of_files, 1, MPI_INT, 0, ControlTag::NoOfFiles,
			 MPI_COMM_WORLD);
	for (const auto &x: this->owned_files) {
		auto &header = x.first;
		auto &hash = x.second;

		MPI_Send(&header, 1, this->Datatypes["FileHeader"], 0,
				 ControlTag::InfoTracker, MPI_COMM_WORLD);
		MPI_Send(hash, header.segments_no, this->Datatypes["FileHash"], 0,
				 ControlTag::InfoTracker, MPI_COMM_WORLD);
	}

	char ok;

	MPI_Recv(&ok, 1, MPI_BYTE, 0, ControlTag::ACK, MPI_COMM_WORLD,
			 MPI_STATUS_IGNORE);
}

/**
 * Current function will handle the full 'downloading' of a file
 * from other seeds & peers.
 */
void Client::HandleDownloadingFile(DownloadingFile &file)
{
	pthread_mutex_lock(&downloading_mutex);
	current_downloading = &file;
	pthread_mutex_unlock(&downloading_mutex);

	int step = 0;
	int segments_to_receive = file.header.segments_no;

	// Array for mantaining the peers/seeds for downloading segments
	int *peers = new int[numProcs - 1]();
	int peers_cnt = 0;

#ifdef ROUND_ROBIN
	int round_robin_cnt = 0;
#endif /* ROUND_ROBIN */

#ifdef DEBUG
	std::unordered_map<int, int> segments_from_client;
#endif

	MPI_Status status;

	char *char_vec_filename = file.header.filename;
	int char_vec_size = strnlen(char_vec_filename, MAX_FILENAME) + 1;

	int SEGMENTS_FAILED = 0;
	while (segments_to_receive > 0) {
		if (step % 10 == 0) {
			// Make a request to the tracker to get info about
			// what peers/seeds have the same file

			MPI_Send(char_vec_filename, char_vec_size, MPI_CHAR, TRACKER_RANK,
					 ControlTag::WhoHasThisFile,
					 MPI_COMM_WORLD);
			MPI_Recv(peers, numProcs, MPI_INT, TRACKER_RANK,
					 ControlTag::WhoHasThisFile, MPI_COMM_WORLD, &status);

			MPI_Get_count(&status, MPI_INT, &peers_cnt);
		}

#ifdef ROUND_ROBIN
		// Perform round robin algorithm for  the current downloading segment
		do
		{
			round_robin_cnt = (round_robin_cnt + 1) % peers_cnt;
			// Try to "get" the segment-file from a peer
			if (GetFileWithGivenHash(peers[round_robin_cnt], file.hashes[step]))
			{
				pthread_mutex_lock(&downloading_mutex);
				file.downloaded[step] = true;
				pthread_mutex_unlock(&downloading_mutex);
				#ifdef DEBUG
				if (segments_from_client.find(peers[round_robin_cnt]) !=
											segments_from_client.end()) {
					segments_from_client[peers[round_robin_cnt]]++;
				} else {
					segments_from_client[peers[round_robin_cnt]] = 1;
				}
				#endif /* DEBUG */
				break;
			}
			else
			{
				SEGMENTS_FAILED++;
			}

		} while (true);
#endif /* ROUND_ROBIN */

#ifdef BUSY_SCORE
		// Get scores associated with the peers & seeds
		auto scores_and_peers = GetScoresForPeers(peers, peers_cnt);

		// Start with the peer having the lowest score and try to
		// request the current segment from it 
		std::set < std::pair < char, int >> ::iterator
		it = scores_and_peers.begin();
		do {
			if (it == scores_and_peers.end())
				it = scores_and_peers.begin();

			// Try to "get" the segment-file from a peer
			if (GetFileWithGivenHash(it->second, file.hashes[step])) {
				pthread_mutex_lock(&downloading_mutex);
				file.downloaded[step] = true;
				pthread_mutex_unlock(&downloading_mutex);
				#ifdef DEBUG
				if (segments_from_client.find(it->second) !=
											segments_from_client.end()) {
					segments_from_client[it->second]++;
				} else {
					segments_from_client[it->second] = 1;
				}
				#endif /* DEBUG */
				break;
			} else {
				it = std::next(it);
				SEGMENTS_FAILED++;
			}
		} while (true);
#endif /* BUSY_SCORE */

		segments_to_receive--;
		step++;
	}

	delete[] peers;
	pthread_mutex_lock(&downloading_mutex);

	// Finish file downloading by considering it an "fully owned" file
	this->owned_files.push_back(file.ConvertToDownloaded());
	current_downloading = nullptr;

	pthread_mutex_unlock(&downloading_mutex);

#ifdef DEBUG
	std::cout << "[Client " << rank << "]NO of Segments received from clients: " << "{ ";
	for (const auto &[k, v]: segments_from_client) {
		std::cout << "[" << k << ": " << v << "], ";
	}
	std::cout << "}\n";
	std::cout << "SEGMENTS FAILED in " << rank << " : " << SEGMENTS_FAILED << "\n";
#endif /* DEBUG */
}

/**
 * This function will simulate the request to a client for a specific segment
 * (represented by its hash). When the answer received from the other client
 * is ACK, we will consider that the segment was fully downloaded.
 */
bool Client::GetFileWithGivenHash(int client, const FileHash &hash)
{

	int bytes_cnt =
	strnlen(current_downloading->header.filename, MAX_FILENAME) + 1;

	// When making request, we will use a fileID (in this case the filename) and the hash
	MPI_Send(current_downloading->header.filename, bytes_cnt, MPI_CHAR, client,
			 ReqFile, MPI_COMM_WORLD);
	MPI_Send(&hash, 1, Datatypes["FileHash"], client,
			 ReqFile, MPI_COMM_WORLD);

	char ans;
	MPI_Recv(&ans, 1, MPI_CHAR, client,
			 AnsFile, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	return ans == ControlTag::ACK;
}

/**
 * Request hashes for a file (represented by its filename) from the
 * tracker.
 */
std::pair<int, FileHash *>
Client::RequestFileDetails(const std::string &filename)
{
	int cnt = 0;
	int bytes_cnt = filename.size() + 1;
	MPI_Status status;

	MPI_Send(filename.c_str(), bytes_cnt, MPI_CHAR, TRACKER_RANK,
			 ControlTag::GiveMeHashes, MPI_COMM_WORLD);

	MPI_Recv(&cnt, 1, MPI_INT, TRACKER_RANK, ControlTag::GiveMeHashes,
			 MPI_COMM_WORLD, &status);

	FileHash *hashes = new FileHash[cnt];
	MPI_Recv(hashes, cnt, Datatypes["FileHash"], TRACKER_RANK,
			 ControlTag::GiveMeHashes, MPI_COMM_WORLD, &status);

	return std::make_pair(cnt, hashes);
}

/**
 * Send a message to the tracker that the client has finished to download
 * a file such that it can be considered a seed.
 */
void Client::SendMessageForFileDownloaded(const std::string &filename)
{
	int bytes_cnt = filename.size() + 1;
	MPI_Send(filename.c_str(), bytes_cnt, MPI_CHAR, TRACKER_RANK,
			 ControlTag::FinishedFileDownload, MPI_COMM_WORLD);
}

/**
 * Checks whether the client has or has not a specific hash for a file.
 * Equivalent with having or not a specific segment.
 */
bool Client::CheckExistingSegment(const char *filename, const FileHash &data)
{

	int index;
	std::vector < std::pair < FileHeader, FileHash * >> ::iterator it;

	pthread_mutex_lock(&downloading_mutex);

	for (it = owned_files.begin(); it != owned_files.end(); it++) {
		// Search for the file with the given filename
		if (it->first.MatchesFilename(filename)) {
			break;
		}
	}
	// File not found in the already downloaded array
	if (it == owned_files.end()) {
		// Has to check whether the requested filename
		// is the file that the client is currently downloading.

		// If the currently downloading file has a different name,
		// therefore the client doesn't have it.
		if (!this->current_downloading ||
			!this->current_downloading->header
			.MatchesFilename(filename)) {
			bool ans = false;
			pthread_mutex_unlock(&downloading_mutex);
			return ans;
		}

		// Else, search for the index of the requested hash.
		for (index = 0;
			 index < current_downloading->header.segments_no; index++) {
			if (current_downloading->hashes[index] == data)
				break;
		}

		// If it's not downloaded, return false. Else, return true.
		if (index == current_downloading->header.segments_no ||
			!current_downloading->downloaded[index]) {
			bool ans = false;
			pthread_mutex_unlock(&downloading_mutex);
			return ans;
		} else {
			bool ans = true;
			pthread_mutex_unlock(&downloading_mutex);
			return ans;
		}
	}
	pthread_mutex_unlock(&downloading_mutex);

	// The hash should be present in a fully owned file.
	auto hashes = it->second;
	auto hashes_size = it->first.segments_no;

	for (index = 0; index < hashes_size; index++) {
		// The hash really exists, return true (equivalent with
		// sending the file).
		if (hashes[index] == data)
			return true;
	}

	return false;
}

/**
 * Make requests to all other peers that have the current segment
 * of the file.
 */
std::set <std::pair<char, int>>
Client::GetScoresForPeers(int *peers, int peers_cnt)
{
	char score;
	MPI_Status status;

	for (int i = 0; i < peers_cnt; i++) {
		MPI_Send(&score, 1, MPI_CHAR, peers[i],
				 ControlTag::HowBusyReq, MPI_COMM_WORLD);
	}

	std::set <std::pair<char, int>> ans;

	for (int i = 0; i < peers_cnt; i++) {
		MPI_Recv(&score, 1, MPI_CHAR, MPI_ANY_SOURCE,
				 ControlTag::HowBusyAns, MPI_COMM_WORLD, &status);

		ans.insert(std::make_pair(score, status.MPI_SOURCE));
	}

	return ans;
}