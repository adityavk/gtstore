#ifndef GTSTORE_COMMON
#define GTSTORE_COMMON

#include <chrono>

constexpr int MAX_KEY_BYTE_PER_REQUEST = 20;
constexpr int MAX_VALUE_BYTE_PER_REQUEST = 1000;
constexpr const char* MANAGER_SERVER_ADDRESS = "0.0.0.0:50051";
constexpr const char* MANAGER_INTERNAL_SERVER_ADDRESS = "0.0.0.0:50050";

constexpr std::chrono::milliseconds DEFAULT_CLIENT_RPC_DEADLINE = std::chrono::milliseconds(500);

#endif