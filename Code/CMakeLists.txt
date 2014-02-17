#cmake

#Clear Targets.txt and Tests.txt
file(WRITE ${CMAKE_BINARY_DIR}/Targets.txt "")
file(WRITE ${CMAKE_BINARY_DIR}/Tests.txt "")

include (${CMAKE_CURRENT_LIST_DIR}/BuildSystem/CMake/CMAKE_GlobProjects.txt)