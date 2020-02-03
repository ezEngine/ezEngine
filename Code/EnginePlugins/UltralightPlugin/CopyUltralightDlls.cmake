
# Function parameter output for debugging:
#message(STATUS "${MSVC_BUILD_CONFIG_DIR}")
#message(STATUS "${BUILDSYSTEM_PLATFORM_64BIT}")
#message(STATUS "${BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP}")
#message(STATUS "${BUILDSYSTEM_PLATFORM_WINDOWS_UWP}")

# ATTENTION: The parameters (BUILDSYSTEM_...) are not available automatically.
# They must be passed through by the CMakeLists.txt that calls this script.

SET (ULTRALIGHT_BIN_PATH "")
SET (ULTRALIGHT_BIN_EXT "")

if (BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP STREQUAL "ON")

	if (BUILDSYSTEM_PLATFORM_64BIT STREQUAL "ON")
	  SET (ULTRALIGHT_BIN_PATH "win/x64")
    else()
      SET (ULTRALIGHT_BIN_PATH "win/x86")
	endif ()

    SET (ULTRALIGHT_BIN_EXT ".dll")
else()
	#Unknown platform.
endif()


# Ultralight libraries
file(COPY "${ULTRALIGHT_SOURCE}/bin/${ULTRALIGHT_BIN_PATH}/Ultralight${ULTRALIGHT_BIN_EXT}"
        DESTINATION "${ULTRALIGHT_TARGET}/")
file(COPY "${ULTRALIGHT_SOURCE}/bin/${ULTRALIGHT_BIN_PATH}/UltralightCore${ULTRALIGHT_BIN_EXT}"
        DESTINATION "${ULTRALIGHT_TARGET}/")
file(COPY "${ULTRALIGHT_SOURCE}/bin/${ULTRALIGHT_BIN_PATH}/WebCore${ULTRALIGHT_BIN_EXT}"
        DESTINATION "${ULTRALIGHT_TARGET}/")


