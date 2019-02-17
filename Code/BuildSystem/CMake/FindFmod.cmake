
# find the folder into which Fmod has been installed
# conveniently, the Fmod installer will set a registry entry for this, so just look it up

# early out, if this target has been created before
if (TARGET Fmod::LowLevel)
	return()
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
	# UWP builds

	find_path(FMOD_DIR fmod.h
		CMAKE_FIND_ROOT_PATH api/lowlevel/inc
		PATHS
		[HKEY_CURRENT_USER\\Software\\FMOD\ Studio\ API\ Universal\ Windows\ Platform]
	)

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(FMOD_DLL_SUFFIX "_X64.dll")
		set(FMOD_LIB_SUFFIX "_X64.lib")
	else()
		set(FMOD_DLL_SUFFIX "_X86.dll")
		set(FMOD_LIB_SUFFIX "_X86.lib")
	endif()

else()
	# Desktop Windows builds
	
	find_path(FMOD_DIR fmod.h
		CMAKE_FIND_ROOT_PATH api/lowlevel/inc
		PATHS
		[HKEY_CURRENT_USER\\Software\\FMOD\ Studio\ API\ Windows]
	)
	
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(FMOD_DLL_SUFFIX "64.dll")
		set(FMOD_LIB_SUFFIX "64_vc.lib")
	else()
		set(FMOD_DLL_SUFFIX ".dll")
		set(FMOD_LIB_SUFFIX "_vc.lib")
	endif()

endif()

set (FMOD_DIR_STUDIO "${FMOD_DIR}/api/studio")
set (FMOD_DIR_FSBANK "${FMOD_DIR}/api/fsbank")
set (FMOD_DIR_LOWLVL "${FMOD_DIR}/api/lowlevel")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Fmod DEFAULT_MSG FMOD_DIR)

if (FMOD_FOUND)

	add_library(Fmod::LowLevel SHARED IMPORTED)
	set_target_properties(Fmod::LowLevel PROPERTIES IMPORTED_LOCATION "${FMOD_DIR_LOWLVL}/lib/fmod${FMOD_DLL_SUFFIX}")
	set_target_properties(Fmod::LowLevel PROPERTIES IMPORTED_LOCATION_DEBUG "${FMOD_DIR_LOWLVL}/lib/fmodL${FMOD_DLL_SUFFIX}")
	set_target_properties(Fmod::LowLevel PROPERTIES IMPORTED_IMPLIB "${FMOD_DIR_LOWLVL}/lib/fmod${FMOD_LIB_SUFFIX}")
	set_target_properties(Fmod::LowLevel PROPERTIES IMPORTED_IMPLIB_DEBUG "${FMOD_DIR_LOWLVL}/lib/fmodL${FMOD_LIB_SUFFIX}")
	set_target_properties(Fmod::LowLevel PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FMOD_DIR_LOWLVL}/inc")

	add_library(Fmod::FsBank SHARED IMPORTED)
	set_target_properties(Fmod::FsBank PROPERTIES IMPORTED_LOCATION "${FMOD_DIR_FSBANK}/lib/fsbank${FMOD_DLL_SUFFIX}")
	set_target_properties(Fmod::FsBank PROPERTIES IMPORTED_LOCATION_DEBUG "${FMOD_DIR_FSBANK}/lib/fsbankL${FMOD_DLL_SUFFIX}")
	set_target_properties(Fmod::FsBank PROPERTIES IMPORTED_IMPLIB "${FMOD_DIR_FSBANK}/lib/fsbank${FMOD_LIB_SUFFIX}")
	set_target_properties(Fmod::FsBank PROPERTIES IMPORTED_IMPLIB_DEBUG "${FMOD_DIR_FSBANK}/lib/fsbankL${FMOD_LIB_SUFFIX}")
	set_target_properties(Fmod::FsBank PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FMOD_DIR_FSBANK}/inc")
	
	add_library(Fmod::Studio SHARED IMPORTED)
	set_target_properties(Fmod::Studio PROPERTIES IMPORTED_LOCATION "${FMOD_DIR_STUDIO}/lib/fmodstudio${FMOD_DLL_SUFFIX}")
	set_target_properties(Fmod::Studio PROPERTIES IMPORTED_LOCATION_DEBUG "${FMOD_DIR_STUDIO}/lib/fmodstudioL${FMOD_DLL_SUFFIX}")
	set_target_properties(Fmod::Studio PROPERTIES IMPORTED_IMPLIB "${FMOD_DIR_STUDIO}/lib/fmodstudio${FMOD_LIB_SUFFIX}")
	set_target_properties(Fmod::Studio PROPERTIES IMPORTED_IMPLIB_DEBUG "${FMOD_DIR_STUDIO}/lib/fmodstudioL${FMOD_LIB_SUFFIX}")
	set_target_properties(Fmod::Studio PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FMOD_DIR_STUDIO}/inc")
	target_link_libraries(Fmod::Studio INTERFACE Fmod::LowLevel)

endif()


unset (FMOD_DIR_STUDIO)
unset (FMOD_DIR_FSBANK)
unset (FMOD_DIR_LOWLVL)
unset (FMOD_DLL_SUFFIX)
unset (FMOD_LIB_SUFFIX)





