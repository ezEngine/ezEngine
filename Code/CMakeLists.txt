#cmake

#Clear Targets.txt and Tests.txt
file(WRITE ${CMAKE_BINARY_DIR}/Targets.txt "")
file(WRITE ${CMAKE_BINARY_DIR}/Tests.txt "")

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_GlobProjects.txt)