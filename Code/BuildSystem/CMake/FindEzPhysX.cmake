
# find the folder into which PhysX has been installed

# early out, if this target has been created before
if (TARGET ezPhysX::Foundation)
	return()
endif()

set (EZ_PHYSX_SDK "EZ_PHYSX_SDK-NOTFOUND" CACHE PATH "Directory of PhysX installation")
mark_as_advanced(FORCE EZ_PHYSX_SDK)

ez_pull_compiler_and_architecture_vars()

if ((EZ_PHYSX_SDK STREQUAL "EZ_PHYSX_SDK-NOTFOUND") OR (EZ_PHYSX_SDK STREQUAL ""))

	if (CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
		if (EZ_CMAKE_ARCHITECTURE_X86 AND EZ_CMAKE_ARCHITECTURE_32BIT)
			set (EZ_SDK_VERSION "PhysX_4.1.1.27006925-vc15uwp32")
			set (EZ_SDK_URL "https://github.com/ezEngine/thirdparty/releases/download/PhysX-4.1.1/PhysX_4.1.1.27006925-vc15uwp32.zip")
		elseif (EZ_CMAKE_ARCHITECTURE_X86 AND EZ_CMAKE_ARCHITECTURE_64BIT)
			set (EZ_SDK_VERSION "PhysX_4.1.1.27006925-vc15uwp64")
			set (EZ_SDK_URL "https://github.com/ezEngine/thirdparty/releases/download/PhysX-4.1.1/PhysX_4.1.1.27006925-vc15uwp64.zip")
		elseif (EZ_CMAKE_ARCHITECTURE_ARM AND EZ_CMAKE_ARCHITECTURE_32BIT)
			set (EZ_SDK_VERSION "PhysX_4.1.1.27006925-vc15uwparm32")
			set (EZ_SDK_URL "https://github.com/ezEngine/thirdparty/releases/download/PhysX-4.1.1/PhysX_4.1.1.27006925-vc15uwparm32.zip")
		elseif (EZ_CMAKE_ARCHITECTURE_ARM AND EZ_CMAKE_ARCHITECTURE_64BIT)
			set (EZ_SDK_VERSION "PhysX_4.1.1.27006925-vc15uwparm64")
			set (EZ_SDK_URL "https://github.com/ezEngine/thirdparty/releases/download/PhysX-4.1.1/PhysX_4.1.1.27006925-vc15uwparm64.zip")
		endif()
	elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
		if (EZ_CMAKE_ARCHITECTURE_X86 AND EZ_CMAKE_ARCHITECTURE_32BIT)
			set (EZ_SDK_VERSION "PhysX_4.1.1.27006925-vc15win32")
			set (EZ_SDK_URL "https://github.com/ezEngine/thirdparty/releases/download/PhysX-4.1.1/PhysX_4.1.1.27006925-vc15win32.zip")
		elseif (EZ_CMAKE_ARCHITECTURE_X86 AND EZ_CMAKE_ARCHITECTURE_64BIT)
			set (EZ_SDK_VERSION "PhysX_4.1.1.27006925-vc15win64")
			set (EZ_SDK_URL "https://github.com/ezEngine/thirdparty/releases/download/PhysX-4.1.1/PhysX_4.1.1.27006925-vc15win64.zip")
		endif()
	endif()

	if (NOT DEFINED EZ_SDK_VERSION)
		message(FATAL_ERROR "Requested platform not implemented for PhysX.")
	endif()

	set (EZ_SDK_LOCAL_ZIP "${CMAKE_BINARY_DIR}/${EZ_SDK_VERSION}.zip")

	if (NOT EXISTS ${EZ_SDK_LOCAL_ZIP})
		message(STATUS "Downloading '${EZ_SDK_URL}'...")
		file(DOWNLOAD ${EZ_SDK_URL} ${EZ_SDK_LOCAL_ZIP} SHOW_PROGRESS)

		message(STATUS "Extracting '${EZ_SDK_LOCAL_ZIP}'...")  
		execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf ${EZ_SDK_LOCAL_ZIP} WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
	else()
		message(STATUS "Already downloaded '${EZ_SDK_LOCAL_ZIP}'")
	endif()

	set (EZ_PHYSX_SDK "${CMAKE_BINARY_DIR}/${EZ_SDK_VERSION}" CACHE PATH "Directory of PhysX installation" FORCE)

endif()

get_property(EZ_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY EZ_SUBMODULE_PREFIX_PATH)

find_path(EZ_PHYSX_SDK PhysX/include/PxActor.h
  PATHS
  ${CMAKE_SOURCE_DIR}/${EZ_SUBMODULE_PREFIX_PATH}/Code/ThirdParty/PhysX
)

if (CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
	if (EZ_CMAKE_ARCHITECTURE_ARM)
		set(PX_ARCH "uwp.arm")
	else()
		set(PX_ARCH "uwp.x86")
	endif()
	set (PX_COMPILER_SUFFIX ".vc141")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	if (EZ_CMAKE_ARCHITECTURE_ARM)
		message(FATAL_ERROR "Physx does not support arm on desktop.")
	else()
		set(PX_ARCH "win.x86")
	endif()
	set (PX_COMPILER_SUFFIX ".vc141.md")
else()
	message(FATAL_ERROR "Unsupported or not implemented physX platform.")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(PX_FOLDER_SUFFIX "_64")
  set(PX_LIB_SUFFIX "_64.lib")
  set(PX_DLL_SUFFIX "_64.dll")
else()
  set(PX_FOLDER_SUFFIX "_32")
  set(PX_LIB_SUFFIX "_32.lib")
  set(PX_DLL_SUFFIX "_32.dll")
endif()

set (PHYSX_LIB_DIR "${EZ_PHYSX_SDK}/PhysX/bin/${PX_ARCH}${PX_FOLDER_SUFFIX}${PX_COMPILER_SUFFIX}")
set (PHYSX_BIN_DIR "${EZ_PHYSX_SDK}/PhysX/bin/${PX_ARCH}${PX_FOLDER_SUFFIX}${PX_COMPILER_SUFFIX}")
set (PHYSX_INC_DIR "${EZ_PHYSX_SDK}/PhysX/Include")
set (PXSHARED_INC_DIR "${EZ_PHYSX_SDK}/PxShared/include")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ezPhysX DEFAULT_MSG EZ_PHYSX_SDK)

if (ezPhysX_FOUND)

	add_library(ezPhysX::Foundation SHARED IMPORTED)
	set_target_properties(ezPhysX::Foundation PROPERTIES IMPORTED_LOCATION "${PHYSX_BIN_DIR}/release/PhysXFoundation${PX_DLL_SUFFIX}")
	set_target_properties(ezPhysX::Foundation PROPERTIES IMPORTED_LOCATION_DEBUG "${PHYSX_BIN_DIR}/debug/PhysXFoundation${PX_DLL_SUFFIX}")
	set_target_properties(ezPhysX::Foundation PROPERTIES IMPORTED_IMPLIB "${PHYSX_LIB_DIR}/release/PhysXFoundation${PX_LIB_SUFFIX}")
	set_target_properties(ezPhysX::Foundation PROPERTIES IMPORTED_IMPLIB_DEBUG "${PHYSX_LIB_DIR}/debug/PhysXFoundation${PX_LIB_SUFFIX}")
	set_target_properties(ezPhysX::Foundation PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PXSHARED_INC_DIR}")
	ez_uwp_mark_import_as_content(ezPhysX::Foundation)

	add_library(ezPhysX::PVD STATIC IMPORTED)
	set_target_properties(ezPhysX::PVD PROPERTIES IMPORTED_LOCATION "${PHYSX_BIN_DIR}/release/PhysXPvdSDK_static${PX_LIB_SUFFIX}")
	set_target_properties(ezPhysX::PVD PROPERTIES IMPORTED_LOCATION_DEBUG "${PHYSX_BIN_DIR}/debug/PhysXPvdSDK_static${PX_LIB_SUFFIX}")
	set_target_properties(ezPhysX::PVD PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PXSHARED_INC_DIR}")
	target_link_libraries(ezPhysX::PVD INTERFACE ezPhysX::Foundation)

	add_library(ezPhysX::Common SHARED IMPORTED)
	set_target_properties(ezPhysX::Common PROPERTIES IMPORTED_LOCATION "${PHYSX_BIN_DIR}/release/PhysXCommon${PX_DLL_SUFFIX}")
	set_target_properties(ezPhysX::Common PROPERTIES IMPORTED_LOCATION_DEBUG "${PHYSX_BIN_DIR}/debug/PhysXCommon${PX_DLL_SUFFIX}")
	set_target_properties(ezPhysX::Common PROPERTIES IMPORTED_IMPLIB "${PHYSX_LIB_DIR}/release/PhysXCommon${PX_LIB_SUFFIX}")
	set_target_properties(ezPhysX::Common PROPERTIES IMPORTED_IMPLIB_DEBUG "${PHYSX_LIB_DIR}/debug/PhysXCommon${PX_LIB_SUFFIX}")
	set_target_properties(ezPhysX::Common PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PHYSX_INC_DIR}")
	target_link_libraries(ezPhysX::Common INTERFACE ezPhysX::Foundation)
	ez_uwp_mark_import_as_content(ezPhysX::Common)

	add_library(ezPhysX::PhysX SHARED IMPORTED)
	set_target_properties(ezPhysX::PhysX PROPERTIES IMPORTED_LOCATION "${PHYSX_BIN_DIR}/release/PhysX${PX_DLL_SUFFIX}")
	set_target_properties(ezPhysX::PhysX PROPERTIES IMPORTED_LOCATION_DEBUG "${PHYSX_BIN_DIR}/debug/PhysX${PX_DLL_SUFFIX}")
	set_target_properties(ezPhysX::PhysX PROPERTIES IMPORTED_IMPLIB "${PHYSX_LIB_DIR}/release/PhysX${PX_LIB_SUFFIX}")
	set_target_properties(ezPhysX::PhysX PROPERTIES IMPORTED_IMPLIB_DEBUG "${PHYSX_LIB_DIR}/debug/PhysX${PX_LIB_SUFFIX}")
	set_target_properties(ezPhysX::PhysX PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PHYSX_INC_DIR}")
	target_link_libraries(ezPhysX::PhysX INTERFACE ezPhysX::Common)
	ez_uwp_mark_import_as_content(ezPhysX::PhysX)

	add_library(ezPhysX::Cooking SHARED IMPORTED)
	set_target_properties(ezPhysX::Cooking PROPERTIES IMPORTED_LOCATION "${PHYSX_BIN_DIR}/release/PhysXCooking${PX_DLL_SUFFIX}")
	set_target_properties(ezPhysX::Cooking PROPERTIES IMPORTED_LOCATION_DEBUG "${PHYSX_BIN_DIR}/debug/PhysXCooking${PX_DLL_SUFFIX}")
	set_target_properties(ezPhysX::Cooking PROPERTIES IMPORTED_IMPLIB "${PHYSX_LIB_DIR}/release/PhysXCooking${PX_LIB_SUFFIX}")
	set_target_properties(ezPhysX::Cooking PROPERTIES IMPORTED_IMPLIB_DEBUG "${PHYSX_LIB_DIR}/debug/PhysXCooking${PX_LIB_SUFFIX}")
	set_target_properties(ezPhysX::Cooking PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PHYSX_INC_DIR}")
	target_link_libraries(ezPhysX::Cooking INTERFACE ezPhysX::Foundation)
	ez_uwp_mark_import_as_content(ezPhysX::Cooking)

	add_library(ezPhysX::Extensions STATIC IMPORTED)
	set_target_properties(ezPhysX::Extensions PROPERTIES IMPORTED_LOCATION "${PHYSX_LIB_DIR}/release/PhysXExtensions_static${PX_LIB_SUFFIX}")
	set_target_properties(ezPhysX::Extensions PROPERTIES IMPORTED_LOCATION_DEBUG "${PHYSX_LIB_DIR}/debug/PhysXExtensions_static${PX_LIB_SUFFIX}")
	set_target_properties(ezPhysX::Extensions PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PHYSX_INC_DIR}")
	target_link_libraries(ezPhysX::Extensions INTERFACE ezPhysX::PhysX)

	add_library(ezPhysX::Character STATIC IMPORTED)
	set_target_properties(ezPhysX::Character PROPERTIES IMPORTED_LOCATION "${PHYSX_BIN_DIR}/release/PhysXCharacterKinematic_static${PX_LIB_SUFFIX}")
	set_target_properties(ezPhysX::Character PROPERTIES IMPORTED_LOCATION_DEBUG "${PHYSX_BIN_DIR}/debug/PhysXCharacterKinematic_static${PX_LIB_SUFFIX}")
	set_target_properties(ezPhysX::Character PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PHYSX_INC_DIR}")
	target_link_libraries(ezPhysX::Character INTERFACE ezPhysX::PhysX)

endif()

unset (PHYSX_LIB_DIR)
unset (PHYSX_BIN_DIR)
unset (PHYSX_INC_DIR)
unset (PHYSX_LIB_DIR)
unset (PHYSX_BIN_DIR)
unset (PXSHARED_INC_DIR)
unset (PX_FOLDER_SUFFIX)
unset (PX_LIB_SUFFIX)





