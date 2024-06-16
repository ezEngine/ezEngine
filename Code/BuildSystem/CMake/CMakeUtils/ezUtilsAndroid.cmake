# #####################################
# ## ez_list_subdirs(result curdir)
# #####################################

macro(ez_list_subdirs result curdir)
	file(GLOB children RELATIVE ${curdir} ${curdir}/*)
	set(dirlist "")

	foreach(child ${children})
		if(IS_DIRECTORY ${curdir}/${child})
			list(APPEND dirlist ${child})
		endif()
	endforeach()

	set(${result} ${dirlist})
endmacro()

# #####################################
# ## ez_android_add_default_content(<target>)
# #####################################
function(ez_android_add_default_content TARGET_NAME)
	ez_pull_all_vars()

	if(NOT EZ_CMAKE_PLATFORM_ANDROID)
		return()
	endif()

	get_property(EZ_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY EZ_SUBMODULE_PREFIX_PATH)

	set(CONTENT_DIRECTORY_DST "${CMAKE_CURRENT_BINARY_DIR}/package")
	set(CONTENT_DIRECTORY_SRC "${EZ_ROOT}/Data/Platform/Android")

	# Copy content files.
	set(ANDROID_ASSET_NAMES
		"res/mipmap-hdpi/ic_launcher.png"
		"res/mipmap-mdpi/ic_launcher.png"
		"res/mipmap-xhdpi/ic_launcher.png"
		"res/mipmap-xxhdpi/ic_launcher.png")

	foreach(contentFile ${ANDROID_ASSET_NAMES})
		configure_file(${CONTENT_DIRECTORY_SRC}/${contentFile} ${CONTENT_DIRECTORY_DST}/${contentFile} COPYONLY)
	endforeach(contentFile)

	# Create Manifest from template.
	configure_file(${CONTENT_DIRECTORY_SRC}/AndroidManifest.xml ${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml)
	configure_file(${CONTENT_DIRECTORY_SRC}/res/values/strings.xml ${CONTENT_DIRECTORY_DST}/res/values/strings.xml)

	if(NOT ANDROID_NDK)
		message(WARNING "ANDROID_NDK not set")

		if(NOT EXISTS "$ENV{ANDROID_NDK_HOME}")
			message(FATAL_ERROR "ANDROID_NDK_HOME environment variable not set. Please ensure it points to the android NDK root folder.")
		else()
			set(ANDROID_NDK $ENV{ANDROID_NDK_HOME})
		endif()
	endif()

	if(NOT EXISTS "$ENV{ANDROID_HOME}")
		if(NOT EXISTS "$ENV{ANDROID_SDK_ROOT}")
			message(FATAL_ERROR "Could not find ANDROID_HOME or ANDROID_SDK_ROOT environment variables")
		else()
			set(ANDROID_SDK $ENV{ANDROID_SDK_ROOT})
		endif()
	else()
		set(ANDROID_SDK $ENV{ANDROID_HOME})
	endif()

	get_filename_component(ANDROID_BUILD_TOOLS_ROOT "${ANDROID_SDK}/build-tools" ABSOLUTE)
	ez_list_subdirs(AVAILABLE_BUILD_TOOLS ${ANDROID_BUILD_TOOLS_ROOT})
	list(LENGTH AVAILABLE_BUILD_TOOLS NUM_AVAILABLE_BUILD_TOOLS)

	if(NUM_AVAILABLE_BUILD_TOOLS EQUAL 0)
		message(FATAL_ERROR "Could not find android build tools in '${ANDROID_SDK}/build-tools' (should contain aapt.exe)")
	else()
		list(SORT AVAILABLE_BUILD_TOOLS)
		list(REVERSE AVAILABLE_BUILD_TOOLS)
		list(GET AVAILABLE_BUILD_TOOLS 0 ANDROID_BUILD_TOOLS)
		set(ANDROID_BUILD_TOOLS "${ANDROID_BUILD_TOOLS_ROOT}/${ANDROID_BUILD_TOOLS}")
	endif()

	string(FIND "${ANDROID_PLATFORM}" "android" out)

	if(${out} EQUAL 0)
		set(ANDROID_PLATFORM_ROOT ${ANDROID_SDK}/platforms/${ANDROID_PLATFORM})
	else()
		set(ANDROID_PLATFORM_ROOT ${ANDROID_SDK}/platforms/android-${ANDROID_PLATFORM})
	endif()

	if(NOT EXISTS "${ANDROID_PLATFORM_ROOT}")
		message(FATAL_ERROR "Android platform '${ANDROID_PLATFORM}' is not installed. Please use the sdk manager to install it to '${ANDROID_PLATFORM_ROOT}'")
	endif()

	# We can't use generator expressions for BYPRODUCTS so we need to get the default LIBRARY_OUTPUT_DIRECTORY.
	# As the ninja generator does not use generator expressions this is not an issue as LIBRARY_OUTPUT_DIRECTORY
	# matches the build type set in CMAKE_BUILD_TYPE.
	get_target_property(APK_OUTPUT_DIR ${TARGET_NAME} LIBRARY_OUTPUT_DIRECTORY)

	STRING(FIND "${APK_OUTPUT_DIR}" "NOTFOUND" APK_OUTPUT_DIR_NOTFOUND)

	if(${APK_OUTPUT_DIR_NOTFOUND} GREATER -1)
		SET(APK_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
	endif()

	add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
		BYPRODUCTS "${APK_OUTPUT_DIR}/${TARGET_NAME}.apk" "${APK_OUTPUT_DIR}/${TARGET_NAME}.unaligned.apk"
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TARGET_NAME}> ${CONTENT_DIRECTORY_DST}/lib/${ANDROID_ABI}/lib${TARGET_NAME}.so
		COMMAND ${CMAKE_COMMAND} -E copy "${EZ_VULKAN_VALIDATIONLAYERS_DIR}/${ANDROID_ABI}/libVkLayer_khronos_validation.so" ${CONTENT_DIRECTORY_DST}/lib/${ANDROID_ABI}/libVkLayer_khronos_validation.so
		COMMAND pwsh -NoLogo -NoProfile -File ${EZ_ROOT}/Utilities/Android/BuildApk.ps1 -BuildToolsPath "${ANDROID_BUILD_TOOLS}" -ContentDirectory "${CONTENT_DIRECTORY_DST}" -Manifest "${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml" -AndroidPlatformRoot "${ANDROID_PLATFORM_ROOT}" -TargetName "${TARGET_NAME}" -OutDir "${APK_OUTPUT_DIR}" -SignKey "${CONTENT_DIRECTORY_SRC}/debug.keystore" -SignPassword "pass:android"
		USES_TERMINAL
	)
endfunction()
