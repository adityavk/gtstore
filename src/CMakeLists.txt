cmake_minimum_required(VERSION 3.8)
set (CMAKE_CXX_STANDARD 17)

project(gtstore)

include(./grpc.cmake)
message(STATUS CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include-internal
)

# Add server executable
add_executable(storage
    server/storage.cpp
    common/GTManagerInternalServiceClient.cpp
    common/GTStorageServiceClient.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)

add_executable(manager
    server/manager.cpp
    common/GTStorageServiceClient.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)

# Add client library
add_library(GTStoreClient STATIC
    client/client.cpp
    common/GTManagerServiceClient.cpp
    common/GTStorageServiceClient.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)

# Add test executable
add_executable(test_app
    test/test_app.cpp
)

# Add test executable
add_executable(perf_test_app
    test/perf_test_app.cpp
)

# Link gRPC libraries
target_link_libraries(storage
    gtstore_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
)

target_link_libraries(manager
    gtstore_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
)

target_link_libraries(GTStoreClient
    gtstore_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
)

target_link_libraries(test_app
    GTStoreClient
    gtstore_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
)

target_link_libraries(perf_test_app
    GTStoreClient
    gtstore_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
)
