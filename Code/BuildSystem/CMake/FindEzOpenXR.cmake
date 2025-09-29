# find the folder into which the OpenXR loader has been installed

# early out, if this target has been created before
if(TARGET ezOpenXR::Loader)
	return()
endif()

set(EZ_OPENXR_LOADER_DIR "EZ_OPENXR_LOADER_DIR-NOTFOUND" CACHE PATH "Directory of OpenXR loader installation")
set(EZ_OPENXR_HEADERS_DIR "EZ_OPENXR_HEADERS_DIR-NOTFOUND" CACHE PATH "Directory of OpenXR headers installation")
set(EZ_OPENXR_PREVIEW_DIR "" CACHE PATH "Directory of OpenXR preview include root")
set(EZ_OPENXR_REMOTING_DIR "" CACHE PATH "Directory of OpenXR remoting installation")
mark_as_advanced(FORCE EZ_OPENXR_LOADER_DIR)
mark_as_advanced(FORCE EZ_OPENXR_HEADERS_DIR)
mark_as_advanced(FORCE EZ_OPENXR_PREVIEW_DIR)
mark_as_advanced(FORCE EZ_OPENXR_REMOTING_DIR)

ez_pull_compiler_and_architecture_vars()

if((EZ_OPENXR_LOADER_DIR STREQUAL "EZ_OPENXR_LOADER_DIR-NOTFOUND") OR(EZ_OPENXR_LOADER_DIR STREQUAL "") OR(EZ_OPENXR_HEADERS_DIR STREQUAL "EZ_OPENXR_HEADERS_DIR-NOTFOUND") OR(EZ_OPENXR_HEADERS_DIR STREQUAL "") OR(EZ_OPENXR_REMOTING_DIR STREQUAL "EZ_OPENXR_REMOTING_DIR-NOTFOUND") OR(EZ_OPENXR_REMOTING_DIR STREQUAL ""))
	ez_nuget_init()
	execute_process(COMMAND ${NUGET} restore ${EZ_ROOT}/Code/EnginePlugins/OpenXRPlugin/packages.config -PackagesDirectory ${CMAKE_BINARY_DIR}/packages
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
	set(EZ_OPENXR_LOADER_DIR "${CMAKE_BINARY_DIR}/packages/OpenXR.Loader.1.0.10.2" CACHE PATH "Directory of OpenXR loader installation" FORCE)
	set(EZ_OPENXR_HEADERS_DIR "${CMAKE_BINARY_DIR}/packages/OpenXR.Headers.1.0.10.2" CACHE PATH "Directory of OpenXR headers installation" FORCE)
	set(EZ_OPENXR_REMOTING_DIR "${CMAKE_BINARY_DIR}/packages/Microsoft.Holographic.Remoting.OpenXr.2.4.0" CACHE PATH "Directory of OpenXR remoting installation" FORCE)
endif()

if(EZ_CMAKE_PLATFORM_WINDOWS_UWP)
	set(OPENXR_DYNAMIC ON)
	find_path(EZ_OPENXR_HEADERS_DIR include/openxr/openxr.h)

	if(EZ_CMAKE_ARCHITECTURE_ARM)
		if(EZ_CMAKE_ARCHITECTURE_64BIT)
			set(OPENXR_BIN_PREFIX "arm64_uwp")
		else()
			set(OPENXR_BIN_PREFIX "arm_uwp")
		endif()
	else()
		if(EZ_CMAKE_ARCHITECTURE_64BIT)
			set(OPENXR_BIN_PREFIX "x64_uwp")
		else()
			set(OPENXR_BIN_PREFIX "Win32_uwp")
		endif()
	endif()

elseif(EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP)
	set(OPENXR_DYNAMIC ON)
	find_path(EZ_OPENXR_HEADERS_DIR include/openxr/openxr.h)

	if(EZ_CMAKE_ARCHITECTURE_64BIT)
		set(OPENXR_BIN_PREFIX "x64")
		find_path(EZ_OPENXR_REMOTING_DIR build/native/include/openxr/openxr_msft_holographic_remoting.h)
	else()
		set(OPENXR_BIN_PREFIX "Win32")
	endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ezOpenXR DEFAULT_MSG EZ_OPENXR_LOADER_DIR)
find_package_handle_standard_args(ezOpenXR DEFAULT_MSG EZ_OPENXR_HEADERS_DIR)
find_package_handle_standard_args(ezOpenXR DEFAULT_MSG EZ_OPENXR_REMOTING_DIR)

if(EZOPENXR_FOUND)
	add_library(ezOpenXR::Loader SHARED IMPORTED)

	if(OPENXR_DYNAMIC)
		set_target_properties(ezOpenXR::Loader PROPERTIES IMPORTED_LOCATION "${EZ_OPENXR_LOADER_DIR}/native/${OPENXR_BIN_PREFIX}/release/bin/openxr_loader.dll")
		set_target_properties(ezOpenXR::Loader PROPERTIES IMPORTED_LOCATION_DEBUG "${EZ_OPENXR_LOADER_DIR}/native/${OPENXR_BIN_PREFIX}/release/bin/openxr_loader.dll")
	endif()

	set_target_properties(ezOpenXR::Loader PROPERTIES IMPORTED_IMPLIB "${EZ_OPENXR_LOADER_DIR}/native/${OPENXR_BIN_PREFIX}/release/lib/openxr_loader.lib")
	set_target_properties(ezOpenXR::Loader PROPERTIES IMPORTED_IMPLIB_DEBUG "${EZ_OPENXR_LOADER_DIR}/native/${OPENXR_BIN_PREFIX}/release/lib/openxr_loader.lib")

	set_target_properties(ezOpenXR::Loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EZ_OPENXR_HEADERS_DIR}/include")

	if(NOT EZ_OPENXR_PREVIEW_DIR STREQUAL "")
		set_target_properties(ezOpenXR::Loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EZ_OPENXR_HEADERS_DIR}/include")
	endif()

	if(EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP AND EZ_CMAKE_ARCHITECTURE_64BIT)

		add_library(ezOpenXR::Remoting INTERFACE IMPORTED)

		if(EZ_CMAKE_PLATFORM_WINDOWS_UWP)
			list(APPEND REMOTING_ASSETS "${EZ_OPENXR_REMOTING_DIR}/build/native/bin/x64/uwp/RemotingXR.json")
			list(APPEND REMOTING_ASSETS "${EZ_OPENXR_REMOTING_DIR}/build/native/bin/x64/uwp/Microsoft.Holographic.AppRemoting.OpenXr.dll")
		else()
			list(APPEND REMOTING_ASSETS "${EZ_OPENXR_REMOTING_DIR}/build/native/bin/x64/Desktop/RemotingXR.json")
			list(APPEND REMOTING_ASSETS "${EZ_OPENXR_REMOTING_DIR}/build/native/bin/x64/Desktop/Microsoft.Holographic.AppRemoting.OpenXr.dll")
		endif()

		set_target_properties(ezOpenXR::Remoting PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EZ_OPENXR_REMOTING_DIR}/build/native/include")
		set_target_properties(ezOpenXR::Remoting PROPERTIES INTERFACE_SOURCES "${REMOTING_ASSETS}")

		set_property(SOURCE ${REMOTING_ASSETS} PROPERTY VS_DEPLOYMENT_CONTENT 1)
		set_property(SOURCE ${REMOTING_ASSETS} PROPERTY VS_DEPLOYMENT_LOCATION "")

	endif()

	
endif()

unset(OPENXR_DYNAMIC)
unset(OPENXR_BIN_PREFIX)
