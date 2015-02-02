#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

# Pass a boolean expression into this macro to build this project only if the expression evaluates to true.
BUILD_CONDITIONAL(BUILDSYSTEM_PLATFORM_WINDOWS)

SET (LINK_LIBRARIES 
  Foundation
  Core
)

if (BUILDSYSTEM_COMPILER_MSVC_110 OR BUILDSYSTEM_COMPILER_MSVC_120)

  if (BUILDSYSTEM_PLATFORM_64BIT)
  
    SET (LINK_LIBRARIES
      ${LINK_LIBRARIES}
      debug ${CMAKE_SOURCE_DIR}/Code/ThirdParty/SFML/libs/vs2012x64/sfml-system-s-d.lib
      debug ${CMAKE_SOURCE_DIR}/Code/ThirdParty/SFML/libs/vs2012x64/sfml-window-s-d.lib
      optimized ${CMAKE_SOURCE_DIR}/Code/ThirdParty/SFML/libs/vs2012x64/sfml-system-s.lib
      optimized ${CMAKE_SOURCE_DIR}/Code/ThirdParty/SFML/libs/vs2012x64/sfml-window-s.lib
    )

  else()

    SET (LINK_LIBRARIES
      ${LINK_LIBRARIES}
      debug ${CMAKE_SOURCE_DIR}/Code/ThirdParty/SFML/libs/vs2012x32/sfml-system-s-d.lib
      debug ${CMAKE_SOURCE_DIR}/Code/ThirdParty/SFML/libs/vs2012x32/sfml-window-s-d.lib
      optimized ${CMAKE_SOURCE_DIR}/Code/ThirdParty/SFML/libs/vs2012x32/sfml-system-s.lib
      optimized ${CMAKE_SOURCE_DIR}/Code/ThirdParty/SFML/libs/vs2012x32/sfml-window-s.lib
    )

  endif()

endif()

if (BUILDSYSTEM_COMPILER_MSVC_100)

  if (BUILDSYSTEM_PLATFORM_64BIT)
  
    SET (LINK_LIBRARIES
      ${LINK_LIBRARIES}
      debug ${CMAKE_SOURCE_DIR}/Code/ThirdParty/SFML/libs/vs2010x64/sfml-system-s-d.lib
      debug ${CMAKE_SOURCE_DIR}/Code/ThirdParty/SFML/libs/vs2010x64/sfml-window-s-d.lib
      optimized ${CMAKE_SOURCE_DIR}/Code/ThirdParty/SFML/libs/vs2010x64/sfml-system-s.lib
      optimized ${CMAKE_SOURCE_DIR}/Code/ThirdParty/SFML/libs/vs2010x64/sfml-window-s.lib
    )

  else()

    SET (LINK_LIBRARIES
      ${LINK_LIBRARIES}
      debug ${CMAKE_SOURCE_DIR}/Code/ThirdParty/SFML/libs/vs2010x32/sfml-system-s-d.lib
      debug ${CMAKE_SOURCE_DIR}/Code/ThirdParty/SFML/libs/vs2010x32/sfml-window-s-d.lib
      optimized ${CMAKE_SOURCE_DIR}/Code/ThirdParty/SFML/libs/vs2010x32/sfml-system-s.lib
      optimized ${CMAKE_SOURCE_DIR}/Code/ThirdParty/SFML/libs/vs2010x32/sfml-window-s.lib
    )

  endif()

endif()

# Someone else finish the MacOS and Linux setup...

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Library.txt)