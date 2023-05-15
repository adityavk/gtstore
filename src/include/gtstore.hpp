#ifndef GTSTORE
#define GTSTORE

#include "common.hpp"
#include <string>

class GTStoreManager {
public:
	void init(int numStorageNodes, int numReplicas);
};

class GTStoreStorage {
public:
	void init(int id, const std::string& server_address);
};

#endif
