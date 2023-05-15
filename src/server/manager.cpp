#include <iostream>
#include <unordered_map>
#include <mutex>
#include <algorithm>
#include <numeric>
#include <thread>
#include <functional>
#include "gtstore.hpp"
#include "argutils.hpp"
#include "GTStorageServiceClient.hpp"

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include "gtstore.grpc.pb.h"

using namespace std;
using grpc::CallbackServerContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerUnaryReactor;
using grpc::Status;
using gtstore::GTManagerService;
using gtstore::GTManagerInternalService;
using gtstore::GetPrimaryNodesResponse;
using gtstore::GetPrimaryNodeRequest;
using gtstore::GetPrimaryNodeResponse;
using gtstore::RegisterStorageNodeRequest;
using gtstore::RegisterStorageNodeResponse;
using gtstore::StorageNode;
using google::protobuf::Empty;

constexpr chrono::milliseconds kHeartbeatInterval = chrono::milliseconds(250);

class GTManagerInternalServiceImpl final : public GTManagerInternalService::CallbackService {
private:
	int numStorageNodes;
	int numReplicas;
	bool initialized = false;
	mutex mMutex;
	unordered_map<int, GTStorageServiceClient> mStorageNodeMap;
	vector<int> aliveStorageNodes;
	int nextPrimaryNodeIndex = 0;
	function<void(vector<int>)> mFailureExternalHandler;
	thread heartbeatThread;
	vector<pair<ServerUnaryReactor*, RegisterStorageNodeResponse*>> mPendingResponses;

	void heartbeat() {
		while (true) {
			this_thread::sleep_for(kHeartbeatInterval);
			refreshStorageServers();
		}
	}
public:
	explicit GTManagerInternalServiceImpl(int numStorageNodes, int numReplicas): numStorageNodes(numStorageNodes), numReplicas(numReplicas), mStorageNodeMap(numStorageNodes) {
		aliveStorageNodes.reserve(numStorageNodes);
		mPendingResponses.reserve(numStorageNodes);
	}

	pair<int, vector<int>> newPrimaryReplicaNodes() {
		lock_guard<mutex> lock(mMutex);
		int primaryId = aliveStorageNodes[nextPrimaryNodeIndex];
		auto aliveNodes = aliveStorageNodes.size();
		nextPrimaryNodeIndex = (nextPrimaryNodeIndex + 1) % aliveNodes;
		vector<int> nodeIds(numReplicas - 1);
		for (int i = 0; i < numReplicas - 1; ++i) {
			nodeIds[i] = aliveStorageNodes[(nextPrimaryNodeIndex + i) % aliveNodes];
		}
		return {primaryId, move(nodeIds)};
	}

	void refreshStorageServers() {
		unique_lock<mutex> lock(mMutex);
			vector<int> deadStorageNodes;
			auto it = aliveStorageNodes.begin();
			int processedNodes = 0;
			while (it != aliveStorageNodes.end()) {
				int id = *it;
				if (!mStorageNodeMap.at(id).isAlive()) {
					if (processedNodes < nextPrimaryNodeIndex) {
						nextPrimaryNodeIndex--;
					}
					it = aliveStorageNodes.erase(it);
					mStorageNodeMap.erase(id);
					deadStorageNodes.push_back(id);
				} else {
					++it;
				}
				++processedNodes;
			}
			lock.unlock();
			if (deadStorageNodes.size() > 0) {
				mFailureExternalHandler(deadStorageNodes);
			}
	}

	void setExternalFailureHandler(function<void(vector<int>)> handler) {
		mFailureExternalHandler = move(handler);
	}

	bool isReady() {
		lock_guard<mutex> lock(mMutex);
		return initialized;
	}

	vector<pair<int, string>> storageNodes() {
		lock_guard<mutex> lock(mMutex);
		vector<pair<int, string>> nodes;
		nodes.reserve(mStorageNodeMap.size());
		for (const auto& [id, node]: mStorageNodeMap) {
			nodes.emplace_back(id, node.url());
		}
		return nodes;
	}

	bool setPrimaryNode(const string& key, int primaryId, const vector<int>& replicaIds) {
		lock_guard<mutex> lock(mMutex);
		return mStorageNodeMap.at(primaryId).setPrimary(key, replicaIds);
	}
  
	ServerUnaryReactor* RegisterStorageNode(CallbackServerContext* context, const RegisterStorageNodeRequest* request, RegisterStorageNodeResponse* response) override {
		lock_guard<mutex> lock(mMutex);
		int id = request->id();
		aliveStorageNodes.push_back(id);
		mStorageNodeMap.emplace(piecewise_construct, forward_as_tuple(id), forward_as_tuple(id, request->url()));
		ServerUnaryReactor* reactor = context->DefaultReactor();
		mPendingResponses.push_back(make_pair(reactor, response));
		if (mStorageNodeMap.size() == numStorageNodes) {
			initialized = true;
			heartbeatThread = thread(&GTManagerInternalServiceImpl::heartbeat, this);
			vector<StorageNode> storageNodes;
			storageNodes.reserve(numStorageNodes);
			StorageNode storageNode;
			for (const auto& [id, node]: mStorageNodeMap) {
				storageNode.set_id(id);
				storageNode.set_url(node.url());
				storageNodes.push_back(storageNode);
			}
			for (const auto& node: storageNodes) {
			}
			for (auto& response: mPendingResponses) {
				*(response.second->mutable_storagenodes()) = {storageNodes.begin(), storageNodes.end()};
				response.first->Finish(Status::OK);
			}
		}
		return reactor;
	}

	virtual ~GTManagerInternalServiceImpl() {
		if (heartbeatThread.joinable()) {
			heartbeatThread.join();
		}
	}
};

class GTManagerServiceImpl final : public GTManagerService::Service {
private:
	// PrimaryKeyTableValue stores the primary node id and the list of replica node ids
	using PrimaryKeyTableValue = pair<int, vector<int>>;
	recursive_mutex mMutex;
	unordered_map<string, PrimaryKeyTableValue> mPrimaryKeyTable;
	unordered_map<int, int> mPrimaryCountMap;
	GTManagerInternalServiceImpl& internalService;
	bool localReadyCopy = false;
	
	// To be called from a function which holds the mutex
	int assignPrimaryNode(const string& key) {
		auto [primaryId, nodeIds] = internalService.newPrimaryReplicaNodes();
		++mPrimaryCountMap[primaryId];
		internalService.setPrimaryNode(key, primaryId, nodeIds);
		mPrimaryKeyTable.try_emplace(key, primaryId, move(nodeIds));
		return primaryId;
	}

	void handleStorageNodeFailure(int deadNodeId) {
		lock_guard<recursive_mutex> lock(mMutex);
		mPrimaryCountMap.erase(deadNodeId);

		for (auto& [key, value]: mPrimaryKeyTable) {
			// For each key that was assigned to the dead node, reassign the primary node
			// Reassignment happens greedily: for each replica node for the key, we check its primaryNodeCount, and pick the lowest one
			if (value.first != deadNodeId) {
				auto it = find(value.second.begin(), value.second.end(), deadNodeId);
				if (it != value.second.end()) {
					value.second.erase(it);
				}
				continue;
			}
			auto minElementIt = min_element(value.second.begin(), value.second.end(), [this](int i, int j) {
				return mPrimaryCountMap[i] < mPrimaryCountMap[j];
			});
			assert(minElementIt != value.second.end());
			int newPrimaryId = *minElementIt;
			++mPrimaryCountMap[newPrimaryId];
			value.first = newPrimaryId;
			value.second.erase(minElementIt);
			internalService.setPrimaryNode(key, newPrimaryId, value.second);
		}
	}

	bool isReady() {
		if (localReadyCopy) {
			return true;
		}
		localReadyCopy = internalService.isReady();
		return localReadyCopy;
	}
public:
	explicit GTManagerServiceImpl(GTManagerInternalServiceImpl& internalService): internalService(internalService) {
		internalService.setExternalFailureHandler([this](vector<int> deadNodeIds) {
			for (int deadNodeId: deadNodeIds) {
				handleStorageNodeFailure(deadNodeId);
			}
		});
	}
  
	Status GetPrimaryNodes(ServerContext* context, const Empty* request, GetPrimaryNodesResponse* response) override {
		lock_guard<recursive_mutex> lock(mMutex);
		if (!isReady()) {
			return Status(grpc::StatusCode::UNAVAILABLE, "Waiting for all storage nodes to register");
		}
		
		internalService.refreshStorageServers();

		for (const auto& [id, url]: internalService.storageNodes()) {
			auto storageNode = response->add_storagenodes();
			storageNode->set_id(id);
			storageNode->set_url(url);
		}
		
		for (const auto& [key, value]: mPrimaryKeyTable) {
			auto primaryNode = response->add_primarynodes();
			primaryNode->set_key(key);
			primaryNode->set_storagenodeid(value.first);
		}
		
		return Status::OK;
	}
	
	Status GetPrimaryNode(ServerContext* context, const GetPrimaryNodeRequest* request, GetPrimaryNodeResponse* response) override {
		lock_guard<recursive_mutex> lock(mMutex);
		if (!isReady()) {
			return Status(grpc::StatusCode::UNAVAILABLE, "Waiting for all storage nodes to register");
		}

		internalService.refreshStorageServers();
		
		string key = request->key();
		bool assignPrimary = request->has_assignprimary() && request->assignprimary();
		int primaryId = -1;
		auto it = mPrimaryKeyTable.find(key);
		if (it == mPrimaryKeyTable.end()) {
			if (!assignPrimary) {
				return Status(grpc::StatusCode::NOT_FOUND, "Key not found");
			}
			primaryId = assignPrimaryNode(key);
		} else {
			primaryId = it->second.first;
		}
		response->set_id(primaryId);
		
		return Status::OK;
	}
};

void GTStoreManager::init(int numStorageNodes, int numReplicas) {
	cout << "Inside GTStoreManager::init()"<< endl;
	
	GTManagerInternalServiceImpl internalService(numStorageNodes, numReplicas);
	thread managerInternalServiceThread([&internalService] {
		string internalServerAddres(MANAGER_INTERNAL_SERVER_ADDRESS);

		ServerBuilder builder;
		builder.AddListeningPort(internalServerAddres, grpc::InsecureServerCredentials());
		builder.RegisterService(&internalService);
		std::unique_ptr<Server> server(builder.BuildAndStart());
		cout << "Manager internal service listening on " << internalServerAddres << std::endl;
		server->Wait();
	});

	thread managerServiceThread([&internalService] {
		string server_address(MANAGER_SERVER_ADDRESS);
		GTManagerServiceImpl service(internalService);

		ServerBuilder builder;
		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
		builder.RegisterService(&service);
		std::unique_ptr<Server> server(builder.BuildAndStart());
		cout << "Manager service listening on " << server_address << std::endl;
		server->Wait();
	});
	managerServiceThread.join();
	managerInternalServiceThread.join();
}

int main(int argc, char **argv) {
	const auto args = parseArgs(argc, argv);
	const string helpMessage = "Usage: ./manager --nodes <num_storage_nodes> --rep <num_replicas>\n";
	int nodes = -1;
	int reps = -1;
	if (!parseIntArg(args, "nodes", nodes) || !parseIntArg(args, "rep", reps)) {
		cout << helpMessage;
		return -1;
	}
	GTStoreManager manager;
	manager.init(nodes, reps);
}
