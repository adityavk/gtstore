#include <iostream>
#include "client.hpp"
#include "GTManagerServiceClient.hpp"
#include "GTStorageServiceClient.hpp"

using namespace std;
using grpc::StatusCode;

constexpr int kMaxRetries = 3;

void GTStoreClient::init(int maxStorageRetries) {
	this->maxStorageRetries = maxStorageRetries;
	auto [storageNodes, primaryNodesInfo]  = managerClient.getPrimaryNodes();
	storageClientsMap.reserve(storageNodes.size());
	for (auto& storageNode: storageNodes) {
		storageClientsMap.try_emplace(storageNode.id(), storageNode.id(), move(storageNode.url()));
	}
	for (auto& [key, nodeId]: primaryNodesInfo) {
		primaryNodeIdMap[key] = nodeId;
	}
}

pair<string, int> GTStoreClient::get(const string& key) {
	int retries = maxStorageRetries;
	while (retries-- > 0) {
		int primaryNodeId = ensurePrimaryNode(key, false);
		if (primaryNodeId == -1) {
			cout << "Key not found!\n";
			return {"", -1};
		}

		auto [statusCode, value] = storageClientsMap.at(primaryNodeId).getValue(key);
		if (statusCode == StatusCode::OK) {
			return { value, primaryNodeId };
		} else {
			primaryNodeIdMap.erase(key);
		}
	}
	return { "", -1 };
}

int GTStoreClient::put(const string& key, const string& value) {
	int retries = maxStorageRetries;
	while (retries-- > 0) {
		int primaryNodeId = ensurePrimaryNode(key, true);
		if (primaryNodeId == -1) {
			cout << "Unable to get primary node for the key\n";
			return -1;
		}

		StatusCode statusCode = storageClientsMap.at(primaryNodeId).setValue(key, value);
		if (statusCode == StatusCode::OK) {
			return primaryNodeId;
		} else {
			primaryNodeIdMap.erase(key);
		}
	}
	return -1;
}

int GTStoreClient::ensurePrimaryNode(const string& key, bool assignPrimaryNode) {
	auto it = primaryNodeIdMap.find(key);
	if (it == primaryNodeIdMap.end()) {
		int nodeId = managerClient.getPrimaryNode(key, assignPrimaryNode);
		if (nodeId != -1) {
			primaryNodeIdMap[key] = nodeId;	
		}
		return nodeId;
	}
	return it->second;
}

void GTStoreClient::finalize() {
}