
# find the folder in which Kraut is located

# early out, if this target has been created before
if (TARGET EzKraut::EzKrautGenerator)
	return()
endif()

get_property(EZ_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY EZ_SUBMODULE_PREFIX_PATH)

find_path(EZ_KRAUT_DIR KrautGenerator/KrautGeneratorDLL.h
  PATHS
  ${CMAKE_SOURCE_DIR}/${EZ_SUBMODULE_PREFIX_PATH}/Code/ThirdParty/Kraut
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(KRAUT_LIB_PATH "${EZ_KRAUT_DIR}/vc141win64")
else()
  set(KRAUT_LIB_PATH "${EZ_KRAUT_DIR}/vc141win32")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EzKraut DEFAULT_MSG EZ_KRAUT_DIR)

if (EZKRAUT_FOUND)

	add_library(EzKraut::EzKrautFoundation SHARED IMPORTED)
	set_target_properties(EzKraut::EzKrautFoundation PROPERTIES IMPORTED_LOCATION "${KRAUT_LIB_PATH}/ezKrautFoundation.dll")
	set_target_properties(EzKraut::EzKrautFoundation PROPERTIES IMPORTED_IMPLIB "${KRAUT_LIB_PATH}/ezKrautFoundation.lib")
	set_target_properties(EzKraut::EzKrautFoundation PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EZ_KRAUT_DIR}")

	add_library(EzKraut::EzKrautGenerator SHARED IMPORTED)
	set_target_properties(EzKraut::EzKrautGenerator PROPERTIES IMPORTED_LOCATION "${KRAUT_LIB_PATH}/ezKrautGenerator.dll")
	set_target_properties(EzKraut::EzKrautGenerator PROPERTIES IMPORTED_IMPLIB "${KRAUT_LIB_PATH}/ezKrautGenerator.lib")
	set_target_properties(EzKraut::EzKrautGenerator PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EZ_KRAUT_DIR}")

endif()

mark_as_advanced(FORCE EZ_KRAUT_DIR)

unset (KRAUT_LIB_PATH)





