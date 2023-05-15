#ifndef GTSTORE_CLIENT
#define GTSTORE_CLIENT

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include "common.hpp"
#include "GTManagerServiceClient.hpp"
#include "GTStorageServiceClient.hpp"

constexpr int DEFAULT_MAX_STORAGE_RETRIES = 3;

class GTStoreClient {
private:
	int maxStorageRetries;
	std::string value;
	GTManagerServiceClient managerClient;
	std::unordered_map<std::string, int> primaryNodeIdMap;
	std::unordered_map<int, GTStorageServiceClient> storageClientsMap;
	int ensurePrimaryNode(const std::string& key, bool assignPrimaryNode);
public:

	void init(int maxStorageRetries = DEFAULT_MAX_STORAGE_RETRIES);
	void finalize();

	// Returns the value and the id of the primary storage server
	std::pair<std::string, int> get(const std::string& key);

	// Returns the id of the primary storage server, -1 if some error occurs
	int put(const std::string& key, const std::string& value);
};

#endif
