
# find the folder in which AssImp is located

# early out, if this target has been created before
if (TARGET EzAssImp::EzAssImp)
	return()
endif()

get_property(EZ_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY EZ_SUBMODULE_PREFIX_PATH)

find_path(EZ_ASSIMP_DIR assimp/ai_assert.h
  PATHS
  ${CMAKE_SOURCE_DIR}/${EZ_SUBMODULE_PREFIX_PATH}/Code/ThirdParty/AssImp
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(ASSIMP_LIB_PATH "${EZ_ASSIMP_DIR}/vc142win64")
else()
  set(ASSIMP_LIB_PATH "${EZ_ASSIMP_DIR}/vc142win32")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EzAssImp DEFAULT_MSG EZ_ASSIMP_DIR)

if (EZASSIMP_FOUND)

	add_library(EzAssImp::EzAssImp SHARED IMPORTED)
	set_target_properties(EzAssImp::EzAssImp PROPERTIES IMPORTED_LOCATION "${ASSIMP_LIB_PATH}/assimp-vc142-mt.dll")
	set_target_properties(EzAssImp::EzAssImp PROPERTIES IMPORTED_IMPLIB "${ASSIMP_LIB_PATH}/assimp-vc142-mt.lib")
	set_target_properties(EzAssImp::EzAssImp PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EZ_ASSIMP_DIR}")

endif()

mark_as_advanced(FORCE EZ_ASSIMP_DIR)

unset (ASSIMP_LIB_PATH)





