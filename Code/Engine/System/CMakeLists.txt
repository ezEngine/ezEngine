#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

SET (LINK_LIBRARIES 
  Foundation
  Core
)

if (BUILDSYSTEM_COMPILER_MSVC_120)

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

if (BUILDSYSTEM_PLATFORM_WINDOWS_7)
  SET (LINK_LIBRARIES ${LINK_LIBRARIES} XINPUT9_1_0)
elseif (BUILDSYSTEM_PLATFORM_WINDOWS_UWP)
  SET (LINK_LIBRARIES ${LINK_LIBRARIES} Xinputuap)
else()
  SET (LINK_LIBRARIES ${LINK_LIBRARIES} XINPUT9_1_0)
endif()

# Someone else finish the MacOS and Linux setup...

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")
