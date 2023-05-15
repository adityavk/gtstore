#include "client.hpp"
#include "argutils.hpp"
#include <iostream>

using namespace std;

int main(int argc, char **argv) {
	const auto args = parseArgs(argc, argv);
	const string helpMessage = "Usage:\nGet value for key: ./test_app --get <key>\nPut value for key: ./test_app --put <key> --val <value>\n";
	bool get = false;
	bool set = false;
	string key;
	string value;
	auto getIt = args.find("get");
	if (getIt != args.end()) {
		get = true;
		key = getIt->second;
	}
	auto setIt = args.find("put");
	if (setIt != args.end()) {
		set = true;
		key = setIt->second;
		auto valIt = args.find("val");
		if (valIt == args.end()) {
			cout << helpMessage;
			return -1;
		}
		value = valIt->second;
	}
	// If either none or both of get/set is specified, print help message and exit
	if (get == set) {
		cout << helpMessage;
		return -1;
	}
	GTStoreClient client;
	client.init();
	if (get) {
		const auto [valueForKey, primaryId] = client.get(key);
		if (primaryId == -1) {
			cout << key << " not found!" << endl;
			return -1;
		}
		cout<< key << ", " << valueForKey << ", server "<< primaryId << endl;
	} else {
		int primaryId = client.put(key, value);
		if (primaryId == -1) {
			cout << key << " could not be put!" << endl;
			return -1;
		}
		cout << "OK, server "<< primaryId << endl;
	}
	
	client.finalize();
	return 0;
}
