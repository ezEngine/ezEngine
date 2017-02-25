#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

# Add all the preprocessor definitions that are needed to configure this library

SET (LINK_LIBRARIES

)

if(MSVC)
  add_definitions (-DWIN32)

  if(BUILDSYSTEM_PLATFORM_WINDOWS_UWP)
    SET (LINK_LIBRARIES
      ${LINK_LIBRARIES}
      OneCore.lib
    )
  else()
    SET (LINK_LIBRARIES
      ${LINK_LIBRARIES}
      ws2_32.lib
      winmm.lib
      Kernel32.lib
      runtimeobject.lib
    )
  endif()
endif()

set (BUILDSYSTEM_PREVENT_STATIC_CODE_ANALYSIS true)

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Library.txt)

# This will break clang makefile builds:
if(MSVC)
  SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CPP)
endif()
