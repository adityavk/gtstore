#ifndef GT_MANAGER_SERVICE_CLIENT_H
#define GT_MANAGER_SERVICE_CLIENT_H

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "gtstore.grpc.pb.h"
#include "common.hpp"

class GTManagerServiceClient {
private:
	std::unique_ptr<gtstore::GTManagerService::Stub> stub_;
public:
	using PrimaryNodeInfo = std::pair<std::string, int>;
	GTManagerServiceClient();

	std::pair<std::vector<gtstore::StorageNode>, std::vector<PrimaryNodeInfo>> getPrimaryNodes();
	
	// returns -1 if not found
	int getPrimaryNode(const std::string& key, bool assignPrimaryNode);

    bool registerStorageNode(int id, const std::string& url);
};

#endif // GT_MANAGER_SERVICE_CLIENT_H
