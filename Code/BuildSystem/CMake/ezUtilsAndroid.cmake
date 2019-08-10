######################################
### ez_android_add_default_content(<target>)
######################################
function(ez_android_add_default_content TARGET_NAME)

	ez_pull_all_vars()

	if (NOT EZ_CMAKE_PLATFORM_ANDROID)
		return()
	endif()

  set(CONTENT_DIRECTORY_DST "${CMAKE_CURRENT_BINARY_DIR}/package")
  set(CONTENT_DIRECTORY_SRC "${CMAKE_SOURCE_DIR}/Data/Platform/Android")

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

  if(NOT EXISTS "$ENV{ANDROID_NDK}")
    message(FATAL_ERROR "Could not find ANDROID_NDK environment variable")
  endif()
  set(ANDROID_NDK $ENV{ANDROID_NDK})

  if(NOT EXISTS "$ENV{ANDROID_SDK}")
    message(FATAL_ERROR "Could not find ANDROID_SDK environment variable")
  endif()
  set(ANDROID_SDK $ENV{ANDROID_SDK})

  if(NOT EXISTS "$ENV{ANDROID_BUILD_TOOLS}")
    message(FATAL_ERROR "Could not find ANDROID_BUILD_TOOLS environment variable (should contain aapt.exe)")
  endif()
  set(ANDROID_BUILD_TOOLS $ENV{ANDROID_BUILD_TOOLS})

  set(_AAPT ${ANDROID_BUILD_TOOLS}/aapt.exe)
  set(_APKSIGNER ${ANDROID_BUILD_TOOLS}/apksigner.bat)
  set(_PLATFORM ${ANDROID_SDK}/platforms/android-${ANDROID_PLATFORM}/android.jar)
  set(_ZIPALIGN ${ANDROID_BUILD_TOOLS}/zipalign.exe)
  set(_ADB ${ANDROID_SDK}/platform-tools/adb.exe)

  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TARGET_NAME}> ${CONTENT_DIRECTORY_DST}/lib/${ANDROID_ABI}/lib${TARGET_NAME}.so
    COMMAND ${_AAPT} package --debug-mode -f -M ${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml -S ${CONTENT_DIRECTORY_DST}/res -I ${_PLATFORM} -F $<TARGET_FILE_DIR:${TARGET_NAME}>/${TARGET_NAME}.unaligned.apk ${CONTENT_DIRECTORY_DST}
    COMMAND ${_ZIPALIGN} -f 4 $<TARGET_FILE_DIR:${TARGET_NAME}>/${TARGET_NAME}.unaligned.apk $<TARGET_FILE_DIR:${TARGET_NAME}>/${TARGET_NAME}.apk
    COMMAND ${_APKSIGNER} sign --ks ${CONTENT_DIRECTORY_SRC}/debug.keystore --ks-pass "pass:android" $<TARGET_FILE_DIR:${TARGET_NAME}>/${TARGET_NAME}.apk
    )

endfunction()
