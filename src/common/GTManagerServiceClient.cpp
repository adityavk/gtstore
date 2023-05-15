#include "common.hpp"
#include "GTManagerServiceClient.hpp"

#include <grpc/grpc.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "gtstore.pb.h"

using namespace std;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using gtstore::StorageNode;
using gtstore::GTManagerService;
using gtstore::GetPrimaryNodesResponse;
using gtstore::GetPrimaryNodeRequest;
using gtstore::GetPrimaryNodeResponse;
using google::protobuf::Empty;

GTManagerServiceClient::GTManagerServiceClient(): stub_(GTManagerService::NewStub(grpc::CreateChannel(MANAGER_SERVER_ADDRESS, grpc::InsecureChannelCredentials()))) {}

pair<vector<StorageNode>, vector<GTManagerServiceClient::PrimaryNodeInfo>> GTManagerServiceClient::getPrimaryNodes() {
	ClientContext context;
	context.set_deadline(std::chrono::system_clock::now() + DEFAULT_CLIENT_RPC_DEADLINE);
	Empty request;
	GetPrimaryNodesResponse response;
	Status status = stub_->GetPrimaryNodes(&context, request, &response);
	if (!status.ok()) {
		cout << "GetPrimaryNodes rpc failed, code: "<< status.error_code() << ", detail: "<< status.error_message() << endl;
		return {};
	}
	vector<StorageNode> storageNodes(response.storagenodes().begin(), response.storagenodes().end());
	vector<PrimaryNodeInfo> primaryNodesInfo;
	primaryNodesInfo.reserve(response.primarynodes_size());
	for (const auto& primaryNode: response.primarynodes()) {
		primaryNodesInfo.emplace_back(primaryNode.key(), primaryNode.storagenodeid());
	}
	return make_pair(storageNodes, primaryNodesInfo);
}
	
int GTManagerServiceClient::getPrimaryNode(const string& key, bool assignPrimaryNode) {
	ClientContext context;
	context.set_deadline(std::chrono::system_clock::now() + DEFAULT_CLIENT_RPC_DEADLINE);
	GetPrimaryNodeRequest request;
	request.set_key(key);
	request.set_assignprimary(assignPrimaryNode);
	GetPrimaryNodeResponse response;
	Status status = stub_->GetPrimaryNode(&context, request, &response);
	if (!status.ok()) {
		if (status.error_code() != grpc::StatusCode::NOT_FOUND) {
			cout << "GetPrimaryNode rpc failed, code: "<< status.error_code() << ", detail: "<< status.error_message() << endl;
		}
		return -1;
	}
	return response.id();
}