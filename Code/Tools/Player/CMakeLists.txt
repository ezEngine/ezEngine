#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

BUILD_CONDITIONAL(BUILDSYSTEM_PLATFORM_WINDOWS)

# Add all the libraries that this project needs to link against here
SET (LINK_LIBRARIES
  Foundation
  Core
  System
  ThirdParty
  RendererFoundation
  RendererCore
  RendererDX11
  GameEngine
  FileservePlugin
)

SET (ADDITIONAL_DEPENDENCIES
  ShaderCompilerHLSL
  InspectorPlugin
)

if (BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
  SET (ADDITIONAL_DEPENDENCIES
    ${ADDITIONAL_DEPENDENCIES}
    RenderDocPlugin
  )
endif()

AddPhysX(FALSE)
AddFmod()

# Set this to true, if your application is a real win32 app (not a console app)
SET (IS_WIN32_APPLICATION true)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")
