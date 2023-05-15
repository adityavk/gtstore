#include "client.hpp"
#include "argutils.hpp"
#include <iostream>
#include <string>
#include <chrono>

using namespace std;

int main(int argc, char **argv) {
	const auto args = parseArgs(argc, argv);
	const string helpMessage = "Usage:\nThroughput test: ./perf_test_app --throughput\nLoad-balance tests: ./perf_test_app --lb\n";
	bool tp = false;
	bool lb = false;
	auto tpIt = args.find("throughput");
	if (tpIt != args.end()) {
		tp = true;
	}
	auto lbIt = args.find("lb");
	if (lbIt != args.end()) {
		lb = true;
	}
	// If either none or both of get/set is specified, print help message and exit
	if (tp == lb) {
		cout << helpMessage;
		return -1;
	}

	int numPairs = 100000;
	int printEvery = 5000;
	
	GTStoreClient client;
	client.init();
	if (tp) {
		string key, val, getVal;
		chrono::microseconds duration(0);
		chrono::steady_clock::time_point start;
		for (int i = 0; i < numPairs; i++) {
			key = "key" + to_string(i);
			val = "value" + to_string(i);
			start = chrono::high_resolution_clock::now();
			client.put(key, val);
			getVal = client.get(key).first;
			duration += chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start);
			assert(getVal == val);
			if (i % printEvery == 0) {
				cout << "Throughput test: " << i << " reads and writes done, time elapsed from start: " << duration.count() << " microseconds" << endl;
			}
		}
		cout << "Throughput test: " << duration.count() << " microseconds for 100k reads and 100k writes" << endl;
	} else {
		unordered_map<int, int> serverToNumKeys;
		string key, val;
		for (int i = 0; i < numPairs; i++) {
			key = "key" + to_string(i);
			val = "value" + to_string(i);
			int primaryId = client.put(key, val);
			++serverToNumKeys[primaryId];
			if (i % printEvery == 0) {
				cout << "Load-balance test: " << i << " writes done" << endl;
			}
		}
		cout << "[Load-balance test] printing server to number of keys map:" << endl;
		for (auto& [id, numKeys] : serverToNumKeys) {
			cout << id << ": " << numKeys << endl;
		}
	}

	client.finalize();
	return 0;
}
