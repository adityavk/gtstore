# From https://github.com/grpc/grpc/blob/v1.53.0/examples/cpp/cmake/common.cmake
cmake_minimum_required(VERSION 3.8)

# Find Protobuf installation
# Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
if(CMAKE_CROSSCOMPILING)
find_program(_PROTOBUF_PROTOC protoc)
else()
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()

# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP gRPC::grpc++)
if(CMAKE_CROSSCOMPILING)
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()


# From https://github.com/grpc/grpc/blob/v1.53.0/examples/cpp/route_guide/CMakeLists.txt

# Proto file
get_filename_component(gtstore_proto "./protocol/gtstore.proto" ABSOLUTE)
get_filename_component(gtstore_proto_path "${gtstore_proto}" PATH)

# Generated sources
set(gtstore_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/gtstore.pb.cc")
set(gtstore_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/gtstore.pb.h")
set(gtstore_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/gtstore.grpc.pb.cc")
set(gtstore_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/gtstore.grpc.pb.h")
add_custom_command(
    OUTPUT "${gtstore_proto_srcs}" "${gtstore_proto_hdrs}" "${gtstore_grpc_srcs}" "${gtstore_grpc_hdrs}"
    COMMAND ${_PROTOBUF_PROTOC}
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
    --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
    -I "${gtstore_proto_path}"
    --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
    "${gtstore_proto}"
    DEPENDS "${gtstore_proto}"
)

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# gtstore_grpc_proto
add_library(gtstore_grpc_proto
    ${gtstore_grpc_srcs}
    ${gtstore_grpc_hdrs}
    ${gtstore_proto_srcs}
    ${gtstore_proto_hdrs})

target_link_libraries(gtstore_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})