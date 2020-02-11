
# find the folder into which Fmod has been installed
# conveniently, the Fmod installer will set a registry entry for this, so just look it up

# early out, if this target has been created before
if (TARGET ezFmod::Core)
	return()
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
	# UWP builds

	find_path(EZ_FMOD_DIR api/core/inc/fmod.h
		PATHS
		[HKEY_CURRENT_USER\\Software\\FMOD\ Studio\ API\ Universal\ Windows\ Platform]
	)

	set(FMOD_DLL_SUFFIX ".dll")
	set(FMOD_LIB_SUFFIX ".lib")

else()
	# Desktop Windows builds

	find_path(EZ_FMOD_DIR api/core/inc/fmod.h
		PATHS
		[HKEY_CURRENT_USER\\Software\\FMOD\ Studio\ API\ Windows]
	)

	set(FMOD_DLL_SUFFIX ".dll")
	set(FMOD_LIB_SUFFIX "_vc.lib")

endif()

set (FMOD_DIR_STUDIO "${EZ_FMOD_DIR}/api/studio")
set (FMOD_DIR_FSBANK "${EZ_FMOD_DIR}/api/fsbank")
set (FMOD_DIR_CORE "${EZ_FMOD_DIR}/api/core")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(FMOD_LIB_ARCH "x64")
else()
  set(FMOD_LIB_ARCH "x86")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EzFmod DEFAULT_MSG EZ_FMOD_DIR)

if (EZFMOD_FOUND)

	add_library(ezFmod::Core SHARED IMPORTED)
	set_target_properties(ezFmod::Core PROPERTIES IMPORTED_LOCATION "${FMOD_DIR_CORE}/lib/${FMOD_LIB_ARCH}/fmod${FMOD_DLL_SUFFIX}")
	set_target_properties(ezFmod::Core PROPERTIES IMPORTED_LOCATION_DEBUG "${FMOD_DIR_CORE}/lib/${FMOD_LIB_ARCH}/fmodL${FMOD_DLL_SUFFIX}")
	set_target_properties(ezFmod::Core PROPERTIES IMPORTED_IMPLIB "${FMOD_DIR_CORE}/lib/${FMOD_LIB_ARCH}/fmod${FMOD_LIB_SUFFIX}")
	set_target_properties(ezFmod::Core PROPERTIES IMPORTED_IMPLIB_DEBUG "${FMOD_DIR_CORE}/lib/${FMOD_LIB_ARCH}/fmodL${FMOD_LIB_SUFFIX}")
	set_target_properties(ezFmod::Core PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FMOD_DIR_CORE}/inc")

	add_library(ezFmod::FsBank SHARED IMPORTED)
	set_target_properties(ezFmod::FsBank PROPERTIES IMPORTED_LOCATION "${FMOD_DIR_FSBANK}/lib/${FMOD_LIB_ARCH}/fsbank${FMOD_DLL_SUFFIX}")
	set_target_properties(ezFmod::FsBank PROPERTIES IMPORTED_LOCATION_DEBUG "${FMOD_DIR_FSBANK}/lib/${FMOD_LIB_ARCH}/fsbankL${FMOD_DLL_SUFFIX}")
	set_target_properties(ezFmod::FsBank PROPERTIES IMPORTED_IMPLIB "${FMOD_DIR_FSBANK}/lib/${FMOD_LIB_ARCH}/fsbank${FMOD_LIB_SUFFIX}")
	set_target_properties(ezFmod::FsBank PROPERTIES IMPORTED_IMPLIB_DEBUG "${FMOD_DIR_FSBANK}/lib/${FMOD_LIB_ARCH}/fsbankL${FMOD_LIB_SUFFIX}")
	set_target_properties(ezFmod::FsBank PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FMOD_DIR_FSBANK}/inc")

	add_library(ezFmod::Studio SHARED IMPORTED)
	set_target_properties(ezFmod::Studio PROPERTIES IMPORTED_LOCATION "${FMOD_DIR_STUDIO}/lib/${FMOD_LIB_ARCH}/fmodstudio${FMOD_DLL_SUFFIX}")
	set_target_properties(ezFmod::Studio PROPERTIES IMPORTED_LOCATION_DEBUG "${FMOD_DIR_STUDIO}/lib/${FMOD_LIB_ARCH}/fmodstudioL${FMOD_DLL_SUFFIX}")
	set_target_properties(ezFmod::Studio PROPERTIES IMPORTED_IMPLIB "${FMOD_DIR_STUDIO}/lib/${FMOD_LIB_ARCH}/fmodstudio${FMOD_LIB_SUFFIX}")
	set_target_properties(ezFmod::Studio PROPERTIES IMPORTED_IMPLIB_DEBUG "${FMOD_DIR_STUDIO}/lib/${FMOD_LIB_ARCH}/fmodstudioL${FMOD_LIB_SUFFIX}")
	set_target_properties(ezFmod::Studio PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FMOD_DIR_STUDIO}/inc")
	target_link_libraries(ezFmod::Studio INTERFACE ezFmod::Core)

endif()

mark_as_advanced(FORCE EZ_FMOD_DIR)

unset (FMOD_DIR_STUDIO)
unset (FMOD_DIR_FSBANK)
unset (FMOD_DIR_CORE)
unset (FMOD_DLL_SUFFIX)
unset (FMOD_LIB_SUFFIX)
unset (FMOD_LIB_ARCH)





