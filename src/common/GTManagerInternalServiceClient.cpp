#include "common.hpp"
#include "GTManagerInternalServiceClient.hpp"
#include <mutex>

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
using gtstore::GTManagerInternalService;
using gtstore::RegisterStorageNodeRequest;
using gtstore::RegisterStorageNodeResponse;

GTManagerInternalServiceClient::GTManagerInternalServiceClient():
	stub_(GTManagerInternalService::NewStub(grpc::CreateChannel(MANAGER_INTERNAL_SERVER_ADDRESS, grpc::InsecureChannelCredentials()))) {}

vector<StorageNode> GTManagerInternalServiceClient::registerStorageNode(int id, const string& url) {
	ClientContext context;
	RegisterStorageNodeRequest request;
	request.set_id(id);
	request.set_url(url);
	RegisterStorageNodeResponse response;
	Status status;

	// Example of callback based client call used from https://github.com/grpc/grpc/blob/6847e05dbb8088a918f06e2231a405942b5c002d/examples/cpp/helloworld/greeter_callback_client.cc#L64
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
    stub_->async()->RegisterStorageNode(&context, &request, &response,
		[&mutex, &cv, &done, &status](Status s) {
			status = std::move(s);
			lock_guard<std::mutex> lock(mutex);
			done = true;
			cv.notify_one();
		});

    std::unique_lock<std::mutex> lock(mutex);
    while (!done) {
		cv.wait(lock);
    }
    if (status.ok()) {
		return vector(response.storagenodes().begin(), response.storagenodes().end());
    }
	cout << "RegisterStorageNode async rpc failed, code: "<< status.error_code() << ", detail: "<< status.error_message() << endl;
	return {};
}