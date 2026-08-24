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
