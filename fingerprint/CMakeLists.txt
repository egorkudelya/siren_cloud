cmake_minimum_required(VERSION 3.6)
project(siren_fingerprint CXX)

set(CMAKE_CXX_STANDARD 17)
option(BUILD_CLOUD_TESTS "Build Siren Cloud with tests" TRUE)

add_subdirectory("/usr/local/include/siren_core" ${CMAKE_CURRENT_BINARY_DIR}/siren_core)

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
)

target_link_libraries(thread_pool PUBLIC siren_core)

if (BUILD_CLOUD_TESTS)
    find_package(GTest REQUIRED)
    set(TEST_SRC
        test/thread_pool.cpp
        )
    set(test_libs gtest gtest_main thread_pool)
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