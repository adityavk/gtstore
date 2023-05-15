#ifndef GT_STORAGE_SERVICE_CLIENT_H
#define GT_STORAGE_SERVICE_CLIENT_H

#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <grpcpp/channel.h>
#include "gtstore.grpc.pb.h"


class GTStorageServiceClient {
private:
	int id_;
	std::string url_;
	std::unique_ptr<gtstore::GTStorageService::Stub> stub_;
public:
	GTStorageServiceClient(int id, const std::string& url);

	int id() const;
	std::string url() const;

	std::pair<grpc::StatusCode, std::string> getValue(const std::string& key);

	grpc::StatusCode setValue(const std::string& key, const std::string& value);
	
	bool setPrimary(const std::string& key, const std::vector<int>& replicaNodeIds);

	bool isAlive();
};

#endif // GT_STORAGE_SERVICE_CLIENT_H
