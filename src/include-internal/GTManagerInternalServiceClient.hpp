#ifndef GT_MANAGER_INTERNAL_SERVICE_CLIENT_H
#define GT_MANAGER_INTERNAL_SERVICE_CLIENT_H

#include <memory>
#include <string>
#include <vector>
#include "gtstore.grpc.pb.h"
#include "common.hpp"

class GTManagerInternalServiceClient {
private:
	std::unique_ptr<gtstore::GTManagerInternalService::Stub> stub_;
public:
	GTManagerInternalServiceClient();
    std::vector<gtstore::StorageNode> registerStorageNode(int id, const std::string& url);
};

#endif // GT_MANAGER_INTERNAL_SERVICE_CLIENT_H
