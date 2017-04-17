
# Function parameter output for debugging:
#message(STATUS "${MSVC_BUILD_CONFIG_DIR}")
#message(STATUS "${BUILDSYSTEM_PLATFORM_64BIT}")
#message(STATUS "${FMOD_SOURCE}")
#message(STATUS "${FMOD_TARGET}")

SET (FMOD_EXT "")
SET (FMOD_EXTL "")

if (MSVC_BUILD_CONFIG_DIR STREQUAL "Debug")
  # Enable if you need the debug libraries in a debug build
  #SET (FMOD_EXTL "${FMOD_EXTL}L")
endif ()

if (BUILDSYSTEM_PLATFORM_64BIT STREQUAL "ON")
  SET (FMOD_EXT "${FMOD_EXT}64")
  SET (FMOD_EXTL "${FMOD_EXTL}64")
endif ()

SET (FMOD_EXT "${FMOD_EXT}.dll")
SET (FMOD_EXTL "${FMOD_EXTL}.dll")

# fsbank libraries
file(COPY "${FMOD_SOURCE}/api/fsbank/lib/fsbank${FMOD_EXT}"
        DESTINATION "${FMOD_TARGET}/")
file(COPY "${FMOD_SOURCE}/api/fsbank/lib/libfsbvorbis${FMOD_EXT}"
        DESTINATION "${FMOD_TARGET}/")
# mp3 not included in fmod anymore
#file(COPY "${FMOD_SOURCE}/api/fsbank/lib/libmp3lame${FMOD_EXT}"
#        DESTINATION "${FMOD_TARGET}/")
#file(COPY "${FMOD_SOURCE}/api/fsbank/lib/twolame${FMOD_EXT}"
#        DESTINATION "${FMOD_TARGET}/")

# lowlevel libraries
file(COPY "${FMOD_SOURCE}/api/lowlevel/lib/fmod${FMOD_EXTL}"
        DESTINATION "${FMOD_TARGET}/")
        
# fmod studio libraries
file(COPY "${FMOD_SOURCE}/api/studio/lib/fmodstudio${FMOD_EXTL}"
        DESTINATION "${FMOD_TARGET}/")
        