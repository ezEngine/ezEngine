#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

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

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")
