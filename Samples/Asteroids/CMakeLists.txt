#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

# Pass a boolean expression into this macro to build this project only if the expression evaluates to true.
BUILD_CONDITIONAL(BUILDSYSTEM_PLATFORM_WINDOWS)

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

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
)

SET (ADDITIONAL_DEPENDENCIES
  ShaderCompilerHLSL
  InspectorPlugin
  ParticlePlugin
)

# Set this to true, if your application is a real win32 app (not a console app)
SET (IS_WIN32_APPLICATION true)

# Add content files for UWP version.
if(BUILDSYSTEM_PLATFORM_WINDOWS_UWP)
  #file(GLOB SAMPLE_FILES ../../Data/Samples/Asteroids/*)
  #set_property(SOURCE ${SAMPLE_FILES} PROPERTY VS_DEPLOYMENT_CONTENT 1)
  #set_property(SOURCE ${SAMPLE_FILES} PROPERTY VS_DEPLOYMENT_LOCATION "Data/Samples/Asteroids")
  #file(GLOB BASEDATA_FILES ../../Data/Base/*)
  #set_property(SOURCE ${BASEDATA_FILES} PROPERTY VS_DEPLOYMENT_CONTENT 1)
  #set_property(SOURCE ${BASEDATA_FILES} PROPERTY VS_DEPLOYMENT_LOCATION "Data/Base")

  #set(CONTENT_FILES ${SAMPLE_FILES} ${BASEDATA_FILES})
  #source_group("Content Files" FILES ${CONTENT_FILES})  # Todo: Either recursive folder or just hide
endif()

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Application.txt)
