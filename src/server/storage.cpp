#include <iostream>
#include <unordered_map>
#include <mutex>
#include "gtstore.hpp"
#include "argutils.hpp"
#include "GTManagerInternalServiceClient.hpp"
#include "GTStorageServiceClient.hpp"

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include "gtstore.grpc.pb.h"
#include "gtstore.pb.h"

using namespace std;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using gtstore::GTStorageService;
using gtstore::GetValueRequest;
using gtstore::GetValueResponse;
using gtstore::SetValueRequest;
using gtstore::SetPrimaryRequest;
using google::protobuf::Empty;

struct GTStoreValue {
	string value;
	bool isPrimary;
	vector<int> replicaIds;
};

class GTStorageServiceImpl final : public GTStorageService::Service {
private:
	int id;
	string url;
	mutex mMutex;
	unordered_map<string, GTStoreValue> mStore;
	unordered_map<int, GTStorageServiceClient> peersMap;
	GTManagerInternalServiceClient managerClient;
public:
	explicit GTStorageServiceImpl(int id, const string& url): id(id), url(url) {}
	
	bool registerWithManager() {
		lock_guard<mutex> lock(mMutex);
		auto peers = managerClient.registerStorageNode(id, url);
		for (const auto& peer : peers) {
			peersMap.emplace(peer.id(), GTStorageServiceClient(peer.id(), peer.url()));
		}
		return peers.size() > 0;
	}
  
	Status GetValue(ServerContext* context, const GetValueRequest* request, GetValueResponse* response) override {
		lock_guard<mutex> lock(mMutex);
		auto it = mStore.find(request->key());
		if (it == mStore.end()) {
			return Status(grpc::StatusCode::NOT_FOUND, "Key not found");
		}
		response->set_value(it->second.value);
		return Status::OK;
	}

	Status SetValue(ServerContext* context, const SetValueRequest* request, Empty* response) override {
		lock_guard<mutex> lock(mMutex);
		const string key = request->key();
		auto it = mStore.find(key);
		if (it == mStore.end()) {
			// This is a replica node, otherwise the key would exist when manager would've set the primary
			mStore.emplace(key, GTStoreValue {request->value(), false, {}});
		} else {
			it->second.value = request->value();
			for (const auto& replicaId : it->second.replicaIds) {
				auto peerIt = peersMap.find(replicaId);
				assert(peerIt != peersMap.end());
				peerIt->second.setValue(key, request->value());
			}

		}
		return Status::OK;
	}

	Status SetPrimary(ServerContext* context, const SetPrimaryRequest* request, Empty* response) override {
		lock_guard<mutex> lock(mMutex);
		const string key = request->key();
		vector<int> replicaIds(request->replicaids().begin(), request->replicaids().end());

		auto it = mStore.find(key);
		if (it == mStore.end()) {
			mStore.emplace(key, GTStoreValue {"", true, replicaIds});
		} else {
			it->second.isPrimary = true;
			it->second.replicaIds = move(replicaIds);
		}
		return Status::OK;
	}

	Status CheckHealth(ServerContext* context, const Empty* request, Empty* response) override {
		return Status::OK;
	}
};

void GTStoreStorage::init(int id, const string& server_address) {
	cout << "Inside GTStoreStorage::init()"<< endl;
	
	GTStorageServiceImpl service(id, server_address);
	if (!service.registerWithManager()) {
		cout << "Registration with manager failed. Exiting."<< endl;
		return;
	} else {
		cout << "Registration with manager successful."<< endl;
	}

	ServerBuilder builder;
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	std::unique_ptr<Server> server(builder.BuildAndStart());
	cout << "Storage service listening on " << server_address << std::endl;
	server->Wait();
}

int main(int argc, char **argv) {
	const auto args = parseArgs(argc, argv);
	const string helpMessage = "Usage: ./storage --id <id> --url <storage_service_url>\n";
	int id = -1;
	if (!parseIntArg(args, "id", id)) {
		cout << helpMessage;
		return -1;
	}
	const auto urlIt = args.find("url");
	if (urlIt == args.end()) {
		cout << helpMessage;
		return -1;
	}
	GTStoreStorage storage;
	storage.init(id, urlIt->second);
}
