cmake_minimum_required(VERSION 3.6)
project(siren_fingerprint CXX)

set(CMAKE_CXX_STANDARD 17)
option(BUILD_CLOUD_TESTS "Build Siren Cloud with tests" TRUE)

add_subdirectory("/usr/local/include/siren_core" ${CMAKE_CURRENT_BINARY_DIR}/siren_core)
set(PostgreSQL_ROOT "/var/lib/postgresql")

add_library(thread_pool STATIC
        src/common/common.cpp
        src/common/common.h
        src/common/safe_queue.h

        src/thread_pool/primitives/task.h
        src/thread_pool/primitives/waitable_future.h
        src/thread_pool/primitives/waitable_future.cpp
        src/thread_pool/pool/thread_pool.h
        src/thread_pool/pool/thread_pool.cpp
        src/thread_pool/dispatch/dispatch.cpp
        src/thread_pool/dispatch/dispatch.h
        src/thread_pool/callback/callback.cpp
        src/thread_pool/callback/callback.h

        src/thread_pool/async_manager.h
)

add_library(request_manager STATIC
        src/common/request_manager.h
        src/common/request_manager.cpp
        )

add_library(logger STATIC
        src/logger/logger.h
        src/logger/logger.cpp
        )

add_library(db_abstraction_layer STATIC
        src/storage/abstract_command.h
        src/storage/abstract_command.cpp
        src/storage/abstract_connection.h
        src/storage/abstract_connection.cpp
        src/storage/abstract_connector.h
        src/storage/abstract_connector.cpp
        src/storage/query.h
        src/storage/query.cpp
        src/storage/connection_pool.h
        src/storage/connection_pool.cpp

        src/storage/postgres/postgres_command.h
        src/storage/postgres/postgres_command.cpp
        src/storage/postgres/postgres_connection.h
        src/storage/postgres/postgres_connection.cpp
        src/storage/postgres/postgres_connector.h
        src/storage/postgres/postgres_connector.cpp

        src/storage/elastic/elastic_command.h
        src/storage/elastic/elastic_command.cpp
        src/storage/elastic/elastic_connection.h
        src/storage/elastic/elastic_connection.cpp
        src/storage/elastic/elastic_connector.h
        src/storage/elastic/elastic_connector.cpp
        )

add_library(histogram STATIC
        src/histogram/entry_layout.h
        src/histogram/histogram.h
        src/histogram/histogram.cpp
        )

add_library(wasserstein STATIC "/usr/local/include/wasserstein/wasserstein.h")
set_target_properties(wasserstein PROPERTIES LINKER_LANGUAGE CXX)

find_library(PQXX_LIB pqxx REQUIRED)
find_library(PQ_LIB pq REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(cpr REQUIRED)
find_package(spdlog REQUIRED)
find_package(Boost 1.36.0 REQUIRED)
if(Boost_FOUND)
    target_include_directories(histogram PRIVATE ${Boost_INCLUDE_DIRS})
endif()

target_link_libraries(logger PRIVATE spdlog::spdlog)
target_link_libraries(thread_pool PUBLIC logger siren_core)
target_link_libraries(db_abstraction_layer PUBLIC logger request_manager thread_pool nlohmann_json::nlohmann_json cpr::cpr ${PQXX_LIB} ${PQ_LIB})

target_include_directories(db_abstraction_layer PRIVATE ${PostgreSQL_INCLUDE_DIRS})
target_include_directories(db_abstraction_layer PRIVATE ${CMAKE_BINARY_DIR})

target_link_libraries(histogram PRIVATE db_abstraction_layer logger wasserstein)

if (BUILD_CLOUD_TESTS)
    find_package(GTest REQUIRED)
    add_library(TEST_DEPS STATIC test/common.h test/common.cpp)
    target_link_libraries(TEST_DEPS PUBLIC siren_core)
    set(TEST_SRC
        test/thread_pool.cpp
        test/query.cpp
        test/postgres.cpp
        test/elastic.cpp
        test/connection_pool.cpp
        test/safe_queue.cpp
        )
    set(test_libs TEST_DEPS gtest gtest_main db_abstraction_layer)
    set(i 0)

    function(add_test_file TEST_NAME TEST_FILE)
        add_executable(${TEST_NAME} ${TEST_FILE})
        target_link_libraries(${TEST_NAME} PRIVATE ${test_libs})
    endfunction()

    foreach(file ${TEST_SRC})
        add_test_file("siren_cloud_test${i}" ${file})
        math(EXPR i "${i} + 1")
    endforeach()
endif()