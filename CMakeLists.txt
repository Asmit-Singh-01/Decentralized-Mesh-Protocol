cmake_minimum_required(VERSION 3.16)
project(DecentralizedMeshProtocol CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# Core library sources (excluding application entry point)
set(CORE_SOURCES
    src/esp_now_driver.cpp
    src/mesh_node.cpp
    src/packet_format.cpp
    src/routing_engine.cpp
    src/security_engine.cpp
    src/swarm_orchestrator.cpp
)

# Simulation target
add_executable(mesh_node_sim src/main.cpp ${CORE_SOURCES})

if(MSVC)
    target_compile_options(mesh_node_sim PRIVATE /W4)
else()
    target_compile_options(mesh_node_sim PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Automated protocol test suite
enable_testing()
add_executable(mesh_protocol_tests tests/test_main.cpp ${CORE_SOURCES})

if(MSVC)
    target_compile_options(mesh_protocol_tests PRIVATE /W4)
else()
    target_compile_options(mesh_protocol_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME mesh_protocol_tests COMMAND mesh_protocol_tests)
