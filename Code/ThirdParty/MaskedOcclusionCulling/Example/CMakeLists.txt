cmake_minimum_required( VERSION 2.6 )
project( Example )
set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_CONFIGURATION_TYPES Debug Release)

#
# Compile masked occlusion culling library
#
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. ./MaskedOcclusionCulling) 

#
# Set compiler flags
#
if(MSVC)
	set(CMAKE_CXX_FLAGS "-std=c++11")
else()
	set(CMAKE_CXX_FLAGS "-std=c++11 -m64")
endif()

#
# Build executable
#
add_executable(Example ExampleMain.cpp)

#
# Link with the occlusion culling library
#
target_link_libraries (Example LINK_PUBLIC MaskedOcclusionCulling)