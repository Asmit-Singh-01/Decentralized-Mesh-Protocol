cmake_minimum_required(VERSION 3.16)

project(DecentralizedMeshProtocol CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")

add_executable(mesh_node_sim ${SOURCES})

if(MSVC)
    target_compile_options(mesh_node_sim PRIVATE /W4)
else()
    target_compile_options(mesh_node_sim PRIVATE -Wall -Wextra -Wpedantic)
endif()


# =========================================================
# Native heartbeat tests
# =========================================================

add_executable(mesh_node_heartbeat_test
    tests/mesh_node_heartbeat_test.cpp
    src/mesh_node.cpp
    src/packet_format.cpp
)

if(MSVC)
    target_compile_options(mesh_node_heartbeat_test PRIVATE /W4)
else()
    target_compile_options(
        mesh_node_heartbeat_test
        PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()


# =========================================================
# CTest integration
# =========================================================

enable_testing()

add_test(
    NAME mesh_node_heartbeat
    COMMAND mesh_node_heartbeat_test
)