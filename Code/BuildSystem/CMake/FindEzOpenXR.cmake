
# find the folder into which the OpenXR loader has been installed

# early out, if this target has been created before
if (TARGET ezOpenXR::Loader)
	return()
endif()

set (EZ_OPENXR_DIR "EZ_OPENXR_DIR-NOTFOUND" CACHE PATH "Directory of OpenXR runtime installation")
set (EZ_OPENXR_PREVIEW_DIR "" CACHE PATH "Directory of OpenXR preview include root")
mark_as_advanced(FORCE EZ_OPENXR_DIR)
mark_as_advanced(FORCE EZ_OPENXR_PREVIEW_DIR)

ez_pull_architecture_vars()

if ((EZ_OPENXR_DIR STREQUAL "EZ_OPENXR_DIR-NOTFOUND") OR (EZ_OPENXR_DIR STREQUAL ""))
	ez_nuget_init()
	execute_process(COMMAND ${NUGET} restore ${CMAKE_SOURCE_DIR}/Code/EnginePlugins/OpenXRPlugin/packages.config -PackagesDirectory ${CMAKE_BINARY_DIR}/packages
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
	set (EZ_OPENXR_DIR "${CMAKE_BINARY_DIR}/packages/OpenXR.Loader.1.0.6.2" CACHE PATH "Directory of OpenXR runtime installation" FORCE)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
	# UWP builds
	set (OPENXR_DYNAMIC ON)
	find_path(EZ_OPENXR_DIR include/openxr/openxr.h)

	if (EZ_CMAKE_ARCHITECTURE_ARM)
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(OPENXR_BIN_PREFIX "arm64_uwp")
		else()
			set(OPENXR_BIN_PREFIX "arm_uwp")
		endif()
	else()
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(OPENXR_BIN_PREFIX "x64_uwp")
		else()
			set(OPENXR_BIN_PREFIX "Win32_uwp")
		endif()
	endif()

elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# Desktop Windows builds

	set (OPENXR_DYNAMIC ON)
	find_path(EZ_OPENXR_DIR include/openxr/openxr.h)

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(OPENXR_BIN_PREFIX "x64")
	else()
		set(OPENXR_BIN_PREFIX "Win32")
	endif()

endif()

set (OPENXR_DIR_LOADER "${EZ_OPENXR_DIR}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ezOpenXR DEFAULT_MSG EZ_OPENXR_DIR)

if (EZOPENXR_FOUND)

	add_library(ezOpenXR::Loader SHARED IMPORTED)
	if (OPENXR_DYNAMIC)
		set_target_properties(ezOpenXR::Loader PROPERTIES IMPORTED_LOCATION "${OPENXR_DIR_LOADER}/native/${OPENXR_BIN_PREFIX}/release/bin/openxr_loader.dll")
		set_target_properties(ezOpenXR::Loader PROPERTIES IMPORTED_LOCATION_DEBUG "${OPENXR_DIR_LOADER}/native/${OPENXR_BIN_PREFIX}/release/bin/openxr_loader.dll")
	endif()
	set_target_properties(ezOpenXR::Loader PROPERTIES IMPORTED_IMPLIB "${OPENXR_DIR_LOADER}/native/${OPENXR_BIN_PREFIX}/release/lib/openxr_loader.lib")
	set_target_properties(ezOpenXR::Loader PROPERTIES IMPORTED_IMPLIB_DEBUG "${OPENXR_DIR_LOADER}/native/${OPENXR_BIN_PREFIX}/release/lib/openxr_loader.lib")
	
	set_target_properties(ezOpenXR::Loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OPENXR_DIR_LOADER}/include")
	if (NOT EZ_OPENXR_PREVIEW_DIR STREQUAL "")
		set_target_properties(ezOpenXR::Loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EZ_OPENXR_PREVIEW_DIR}/include")
	endif()

	ez_uwp_mark_import_as_content(ezOpenXR::Loader)

endif()

unset (OPENXR_DYNAMIC)
unset (OPENXR_DIR_LOADER)
unset (OPENXR_BIN_PREFIX)






