#include "file_hash.h"

#include <cstring>

FileHeader::FileHeader(const std::string &filename_, int segments_no_) {
	memccpy(filename, filename_.c_str(), '\0', MAX_FILENAME);
	segments_no = segments_no_;
}

FileHeader::FileHeader() {
	memset(filename, '\0', MAX_FILENAME);
	segments_no = 0;
}

FileHash::FileHash() {
	memset(x, 0, HASH_SIZE);
}

FileHash::FileHash(const std::string &s) {
	memccpy(x, s.c_str(), '\0', HASH_SIZE);
}

std::ostream& operator<< (std::ostream &o, const FileHash &f) {
	for (unsigned i = 0; i < HASH_SIZE; i++) {
		o << f.x[i];
	}
	return o;
}

MPI_Datatype SubscribeFileHeaderTo_MPI() {
	MPI_Datatype custom, old_types[2];
	int block_counts[2];
	MPI_Aint offsets[2];
	

	offsets[0] = offsetof(FileHeader, filename);
	old_types[0] = MPI_CHAR;
	block_counts[0] = MAX_FILENAME;

	offsets[1] = offsetof(FileHeader, segments_no);
	old_types[1] = MPI_INT;
	block_counts[1] = 1;

	MPI_Type_create_struct(2, block_counts, offsets, old_types, &custom);
	MPI_Type_commit(&custom);

	return custom;
}

MPI_Datatype SubscribeFileHashTo_MPI() {
	MPI_Datatype custom_type, old_types[1];
	int blockcounts[1];
	MPI_Aint offsets[1];

	offsets[0] = offsetof(FileHash, x);
	old_types[0] = MPI_CHAR;
	blockcounts[0] = HASH_SIZE;

	MPI_Type_create_struct(1, blockcounts, offsets, old_types, &custom_type);
	MPI_Type_commit(&custom_type);
	
	return custom_type;
}