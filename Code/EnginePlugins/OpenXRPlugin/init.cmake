######################################
### OpenXR support
######################################

set (EZ_BUILD_OPENXR OFF CACHE BOOL "Whether support for OpenXR should be added")

######################################
### ez_fetch_openxr()
######################################

macro(ez_fetch_openxr)
	include(FetchContent)

	# Set RPATH for OpenXR libraries so they can find dependencies in the same directory
	# This must be set before FetchContent_MakeAvailable
	set(CMAKE_BUILD_RPATH "$ORIGIN" CACHE STRING "" FORCE)
	set(CMAKE_INSTALL_RPATH "$ORIGIN" CACHE STRING "" FORCE)

	# openxr_loader - From github.com/KhronosGroup
	set(BUILD_API_LAYERS
			ON
			CACHE INTERNAL "Use OpenXR layers"
	)
	set(BUILD_TESTS
			OFF
			CACHE INTERNAL "Build tests"
	)
	FetchContent_Declare(
			OpenXR
			EXCLUDE_FROM_ALL
			DOWNLOAD_EXTRACT_TIMESTAMP
			URL_HASH MD5=f52248ef83da9134bec2b2d8e0970677
			URL https://github.com/KhronosGroup/OpenXR-SDK-Source/archive/refs/tags/release-1.1.49.tar.gz
			SOURCE_DIR
			openxr
	)

	FetchContent_MakeAvailable(OpenXR)

	# Move OpenXR targets to ThirdParty folder in Visual Studio solution
	set_target_properties(openxr_loader PROPERTIES FOLDER "ThirdParty")
	set_target_properties(XrApiLayer_core_validation PROPERTIES FOLDER "ThirdParty")

	# Suppress warnings in third-party generated code
	if(EZ_CMAKE_COMPILER_GCC OR EZ_CMAKE_COMPILER_CLANG)
		target_compile_options(XrApiLayer_core_validation PRIVATE -Wno-address)
	endif()

endmacro()

######################################
### ez_link_target_openxr(<target>)
######################################

function(ez_link_target_openxr TARGET_NAME)

	target_link_libraries(${TARGET_NAME} PRIVATE openxr_loader)
	add_dependencies(${TARGET_NAME} XrApiLayer_core_validation)

endfunction()

