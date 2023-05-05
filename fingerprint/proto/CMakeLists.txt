cmake_minimum_required(VERSION 3.6)

project(siren_proto)

find_package(absl CONFIG REQUIRED)
find_program(GRPC_CPP_PLUGIN NAMES grpc_cpp_plugin)
find_library(GRPC_LIBRARY NAMES grpc)
find_library(GRPCPP_LIBRARY NAMES grpc++)
find_library(GPR_LIBRARY NAMES gpr)

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

set(GRPC_LIBRARIES ${GRPCPP_LIBRARY} ${GRPC_LIBRARY} ${GPR_LIBRARY})

if(GRPC_LIBRARIES)
    message(STATUS "Found GRPC: ${GRPC_LIBRARIES}; plugin - ${GRPC_CPP_PLUGIN}")
endif()

find_package(Protobuf REQUIRED)

set(GENERATED_PROTOBUF_PATH ${CMAKE_BINARY_DIR})

set(SIREN_PROTO
        google/api/annotations.proto
        google/api/http.proto
        fingerprint.proto
        )

set(GENERATED_PROTOBUF_FILES
        ${GENERATED_PROTOBUF_PATH}/fingerprint.grpc.pb.cc
        ${GENERATED_PROTOBUF_PATH}/fingerprint.grpc.pb.h
        ${GENERATED_PROTOBUF_PATH}/fingerprint.pb.cc
        ${GENERATED_PROTOBUF_PATH}/fingerprint.pb.h

        ${GENERATED_PROTOBUF_PATH}/google/api/annotations.pb.cc
        ${GENERATED_PROTOBUF_PATH}/google/api/annotations.pb.h

        ${GENERATED_PROTOBUF_PATH}/google/api/http.pb.cc
        ${GENERATED_PROTOBUF_PATH}/google/api/http.pb.h
        )

add_custom_command(
        DEPENDS ${SIREN_PROTO}
        OUTPUT ${GENERATED_PROTOBUF_FILES}

        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
        ARGS "--proto_path=${CMAKE_CURRENT_SOURCE_DIR}"
        "--cpp_out=${GENERATED_PROTOBUF_PATH}"
        ${SIREN_PROTO}

        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
        ARGS "--proto_path=${CMAKE_CURRENT_SOURCE_DIR}"
        "--grpc_out=${GENERATED_PROTOBUF_PATH}"
        "--plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}"
        ${SIREN_PROTO}

        COMMENT "Running gRPC C++ protocol buffer compiler on ${SIREN_PROTO}"
        VERBATIM
)

include_directories(${GENERATED_PROTOBUF_PATH})
include_directories(${Protobuf_INCLUDE_DIRS})

add_library(siren_proto STATIC ${GENERATED_PROTOBUF_FILES} ${SIREN_PROTO})

target_link_libraries(siren_proto
        PUBLIC
        absl::synchronization
        ${GRPC_LIBRARIES}
        ${_REFLECTION}
        ${PROTOBUF_LIBRARY}
        ${OPENSSL_LIBRARIES}
        protobuf::libprotobuf
        z
        pthread
        )

