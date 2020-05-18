
# find the folder in which RmlUi is located

# early out, if this target has been created before
if (TARGET EzRmlUi::Core)
	return()
endif()

get_property(EZ_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY EZ_SUBMODULE_PREFIX_PATH)

find_path(EZ_RMLUI_DIR Include/RmlUi/Core.h
  PATHS
  ${CMAKE_SOURCE_DIR}/${EZ_SUBMODULE_PREFIX_PATH}/Code/ThirdParty/RmlUi
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(RMLUI_LIB_PATH "${EZ_RMLUI_DIR}/Bin/win64")
else()
  set(RMLUI_LIB_PATH "${EZ_RMLUI_DIR}/Bin/win32")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EzRmlUi DEFAULT_MSG EZ_RMLUI_DIR)

if (EZRMLUI_FOUND)

	add_library(EzRmlUi::Core SHARED IMPORTED)
	set_target_properties(EzRmlUi::Core PROPERTIES IMPORTED_LOCATION "${RMLUI_LIB_PATH}/RmlCore.dll")
	set_target_properties(EzRmlUi::Core PROPERTIES IMPORTED_IMPLIB "${RMLUI_LIB_PATH}/RmlCore.lib")
	set_target_properties(EzRmlUi::Core PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EZ_RMLUI_DIR}/Include")
  
  add_library(EzRmlUi::Freetype SHARED IMPORTED)
	set_target_properties(EzRmlUi::Freetype PROPERTIES IMPORTED_LOCATION "${RMLUI_LIB_PATH}/freetype.dll")

endif()

mark_as_advanced(FORCE EZ_RMLUI_DIR)

unset (RMLUI_LIB_PATH)





