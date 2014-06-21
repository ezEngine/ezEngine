#cmake

cmake_minimum_required(VERSION 2.8)

project ("ezEngine")

message (STATUS "Adding SubDirectory 'Code'")
add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/Code)

message (STATUS "Adding SubDirectory 'Projects'")
add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/Projects)

message (STATUS "Adding SubDirectory 'Tools'")
add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/Tools)

message (STATUS "Adding SubDirectory 'Samples'")
add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/Samples)

message (STATUS "Adding SubDirectory 'UnitTests'")
add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/UnitTests)