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

# ==========================================
# TEST INTEGRATION
# ==========================================

# 1. Enable CTest support in the project
enable_testing()

# 2. Copy the source files, but remove main.cpp so we don't have duplicate main() functions
set(TEST_SOURCES ${SOURCES})
list(REMOVE_ITEM TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

# 3. Create the test executable
add_executable(test_serialization test/test_serialization.cpp ${TEST_SOURCES})

# 4. Apply the same warnings to the test code
if(MSVC)
    target_compile_options(test_serialization PRIVATE /W4)
else()
    target_compile_options(test_serialization PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 5. Register it as a test
add_test(NAME SerializationTest COMMAND test_serialization)