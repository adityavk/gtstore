#include "common.hpp"
#include "GTStorageServiceClient.hpp"

#include <grpc/grpc.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "gtstore.pb.h"

using namespace std;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::StatusCode;
using gtstore::GTStorageService;
using gtstore::GetValueRequest;
using gtstore::GetValueResponse;
using gtstore::SetValueRequest;
using gtstore::SetPrimaryRequest;
using google::protobuf::Empty;

GTStorageServiceClient::GTStorageServiceClient(int id, const std::string& url): 
	id_(id), url_(url), stub_(GTStorageService::NewStub(grpc::CreateChannel(url, grpc::InsecureChannelCredentials()))) {}

int GTStorageServiceClient::id() const {
	return id_;
}

std::string GTStorageServiceClient::url() const {
	return url_;
}

pair<StatusCode, string> GTStorageServiceClient::getValue(const string& key) {
	ClientContext context;
	context.set_deadline(std::chrono::system_clock::now() + DEFAULT_CLIENT_RPC_DEADLINE);
	GetValueRequest request;
	request.set_key(key);
	GetValueResponse response;
	Status status = stub_->GetValue(&context, request, &response);
	if (!status.ok()) {
		cout << "GetValue rpc failed when sending request for key: " << key << " to storage node: " << url_ << "with error code: " << status.error_code() << ", detail: " << status.error_message() << endl;
	}
	return { status.error_code(), response.value()};
}

StatusCode GTStorageServiceClient::setValue(const string& key, const string& value) {
	ClientContext context;
	context.set_deadline(std::chrono::system_clock::now() + DEFAULT_CLIENT_RPC_DEADLINE);
	SetValueRequest request;
	request.set_key(key);
	request.set_value(value);
	Empty response;
	Status status = stub_->SetValue(&context, request, &response);
	if (!status.ok()) {
		cout << "SetValue rpc failed for key: " << key << " to storage node: " << url_ << "with error code: " << status.error_code() << ", detail: " << status.error_message() << endl;
	}
	return status.error_code();
}

bool GTStorageServiceClient::setPrimary(const std::string& key, const vector<int>& replicaNodeIds) {
	ClientContext context;
	context.set_deadline(std::chrono::system_clock::now() + DEFAULT_CLIENT_RPC_DEADLINE);
	SetPrimaryRequest request;
	request.set_key(key);
	for (int id : replicaNodeIds) {
		request.add_replicaids(id);
	}
	Empty response;
	Status status = stub_->SetPrimary(&context, request, &response);
	if (!status.ok()) {
		cout << "SetPrimary rpc failed for key: " << key << " to storage node: " << url_ << "with error code: " << status.error_code() << ", detail: " << status.error_message() << endl;
		return false;
	}
	return true;
}

bool GTStorageServiceClient::isAlive() {
	ClientContext context;
	context.set_deadline(std::chrono::system_clock::now() + DEFAULT_CLIENT_RPC_DEADLINE);
	Empty request;
	Empty response;
	Status status = stub_->CheckHealth(&context, request, &response);
	if (!status.ok()) {
		cout << "IsAlive rpc failed for storage node: " << url_ << "with error code: " << status.error_code() << ", detail: " << status.error_message() << endl;
		return false;
	}
	return true;
}