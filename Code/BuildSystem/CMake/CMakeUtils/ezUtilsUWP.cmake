# #####################################
# ## ez_uwp_mark_import_as_content(<import>)
# #####################################

function(ez_uwp_mark_import_as_content IMPORT)
	# PLATFORM-TODO
	if(NOT EZ_CMAKE_PLATFORM_WINDOWS_UWP)
		return()
	endif()

	ez_pull_config_vars()

	get_target_property(_dll_location_debug ${IMPORT} IMPORTED_LOCATION_${EZ_BUILDTYPENAME_DEBUG_UPPER})
	get_target_property(_dll_location ${IMPORT} IMPORTED_LOCATION)

	if(${_dll_location_debug} STREQUAL ${_dll_location})
		set_property(SOURCE ${_dll_location_debug} PROPERTY VS_DEPLOYMENT_CONTENT 1)
	else()
		set_property(SOURCE ${_dll_location_debug} PROPERTY VS_DEPLOYMENT_CONTENT $<CONFIG:${EZ_BUILDTYPENAME_DEBUG}>)

		set_property(SOURCE ${_dll_location} PROPERTY VS_DEPLOYMENT_CONTENT $<OR:$<CONFIG:${EZ_BUILDTYPENAME_RELEASE}>,$<CONFIG:${EZ_BUILDTYPENAME_DEV}>>)

		unset(_dll_location_debug)
		unset(_dll_location)
	endif()
endfunction()

# #####################################
# ## ez_uwp_add_import_to_sources(<target_name> <import>)
# #####################################
function(ez_uwp_add_import_to_sources TARGET_NAME IMPORT)
	# PLATFORM-TODO
	if(NOT EZ_CMAKE_PLATFORM_WINDOWS_UWP)
		return()
	endif()

	ez_pull_config_vars()

	get_target_property(_dll_location ${IMPORT} IMPORTED_LOCATION_${EZ_BUILDTYPENAME_DEBUG_UPPER})
	target_sources(${TARGET_NAME} PUBLIC ${_dll_location})

	get_target_property(_dll_location ${IMPORT} IMPORTED_LOCATION_${EZ_BUILDTYPENAME_DEV_UPPER})
	target_sources(${TARGET_NAME} PUBLIC ${_dll_location})

	get_target_property(_dll_location ${IMPORT} IMPORTED_LOCATION_${EZ_BUILDTYPENAME_RELEASE_UPPER})
	target_sources(${TARGET_NAME} PUBLIC ${_dll_location})

	unset(_dll_location)
endfunction()

# #####################################
# ## ez_uwp_add_default_content(<target>)
# #####################################
function(ez_uwp_fix_library_properties TARGET_NAME ALL_SOURCE_FILES)
	# PLATFORM-TODO
	if(NOT EZ_CMAKE_PLATFORM_WINDOWS_UWP)
		return()
	endif()

	get_target_property(targetType ${TARGET_NAME} TYPE)

	if(NOT targetType STREQUAL "STATIC_LIBRARY")
		# Needs to be set or cmake does not deploy the dll as only managed and winrt
		# dlls are set to be deployed by cmake for some arbitrary reason.
		set_property(TARGET ${TARGET_NAME} PROPERTY VS_WINRT_COMPONENT ON)

		# Cmake refuses to deploy C dynamic libraries so we force all of them into C++ mode.
		# This is because C can't be winrt and without it cmake throws the dll away again.
		foreach(FILE ${ALL_SOURCE_FILES})
			get_filename_component(FILE_PATH ${FILE} LAST_EXT)

			if(${FILE_PATH} STREQUAL ".c")
				set_source_files_properties(${FILE} PROPERTIES LANGUAGE CXX)
			endif()
		endforeach()
	endif()
endfunction()

function(ez_uwp_add_default_content TARGET_NAME)
	ez_pull_all_vars()

	# PLATFORM-TODO
	if(NOT EZ_CMAKE_PLATFORM_WINDOWS_UWP)
		return()
	endif()

	get_property(EZ_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY EZ_SUBMODULE_PREFIX_PATH)

	set(CONTENT_DIRECTORY_DST "${CMAKE_CURRENT_BINARY_DIR}/Assets/")
	set(CONTENT_DIRECTORY_SRC "${EZ_ROOT}/Data/Platform/UWP/")

	# Copy content files.
	set(UWP_ASSET_NAMES
		"LockScreenLogo.scale-200.png"
		"SplashScreen.scale-200.png"
		"Square44x44Logo.scale-200.png"
		"Square44x44Logo.targetsize-24_altform-unplated.png"
		"Square150x150Logo.scale-200.png"
		"StoreLogo.png"
		"Wide310x150Logo.scale-200.png"
		"Windows_TemporaryKey.pfx")

	foreach(contentFile ${UWP_ASSET_NAMES})
		configure_file(${CONTENT_DIRECTORY_SRC}${contentFile} ${CONTENT_DIRECTORY_DST}${contentFile} COPYONLY)
		list(APPEND UWP_ASSETS ${CONTENT_DIRECTORY_DST}${contentFile})
	endforeach(contentFile)

	set_property(TARGET ${TARGET_NAME} PROPERTY VS_WINRT_COMPONENT ON)
	set_property(SOURCE ${UWP_ASSETS} PROPERTY VS_DEPLOYMENT_LOCATION "Assets")
	set_property(SOURCE ${UWP_ASSETS} PROPERTY VS_DEPLOYMENT_CONTENT 1)

	# Create Manifest from template.
	get_filename_component(SHORT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)
	set(IDENTITY_NAME "${SHORT_NAME}-${CONFIGURATION_DESC}")
	set(PACKAGE_GUID 3370B74B-62B8-4009-828B-01A02CB4AB56)
	string(UUID PACKAGE_GUID NAMESPACE ${PACKAGE_GUID} NAME "ezEngine" TYPE MD5) # Generate GUID

	# Add spatial perception to appx manifest if we're linking against WindowsMixedReality.
	# if (WindowsMixedReality IN_LIST LINK_LIBRARIES)
	# set(EXTRA_APP_CAPABILITIES "<uap2:Capability Name=\"spatialPerception\"/>")
	# endif()
	configure_file(${CONTENT_DIRECTORY_SRC}/package_template.appxmanifest ${CONTENT_DIRECTORY_DST}Package.appxmanifest @ONLY)

	list(APPEND UWP_ASSETS ${CONTENT_DIRECTORY_DST}Package.appxmanifest)

	# Include all content.
	target_sources(${TARGET_NAME} PRIVATE ${UWP_ASSETS})

	source_group("Assets" FILES ${UWP_ASSETS})
endfunction()