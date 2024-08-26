set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_NAME "UNKNOWN")
set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_PREFIX "")
set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_POSTFIX "")
set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_POSIX OFF)

message(STATUS "'CMAKE_SYSTEM_NAME' is '${CMAKE_SYSTEM_NAME}'")

# automatically include all files in the Platforms subfolder
message(STATUS "Including platform-files in '${CMAKE_CURRENT_LIST_DIR}/Platforms'")
file(GLOB DETECT_FILES "${CMAKE_CURRENT_LIST_DIR}/Platforms/Detect_*.cmake")
foreach(DETECT_FILE ${DETECT_FILES})
	message(STATUS "Including platform-file '${DETECT_FILE}'")
	include("${DETECT_FILE}")
endforeach()

get_property(EZ_CMAKE_PLATFORM_NAME GLOBAL PROPERTY EZ_CMAKE_PLATFORM_NAME)
get_property(EZ_CMAKE_PLATFORM_POSTFIX GLOBAL PROPERTY EZ_CMAKE_PLATFORM_POSTFIX)

if (EZ_CMAKE_PLATFORM_NAME STREQUAL "UNKNOWN")
	message(FATAL_ERROR "Failed to detect platform.")
else()
	message(STATUS "Detected Platform is '${EZ_CMAKE_PLATFORM_NAME}'")
endif()

get_property(EZ_CMAKE_RELPATH GLOBAL PROPERTY EZ_CMAKE_RELPATH)
set(PLATFORM_CFG_FILE "${EZ_ROOT}/${EZ_CMAKE_RELPATH}/Platforms/Configure_${EZ_CMAKE_PLATFORM_POSTFIX}.cmake")
include("${PLATFORM_CFG_FILE}")
