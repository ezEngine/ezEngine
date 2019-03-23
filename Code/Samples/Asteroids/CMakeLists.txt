ez_cmake_init()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
	return()
endif()

# Add content files for UWP version.
if(EZ_CMAKE_PLATFORM_WINDOWS_UWP)
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

ez_make_winmain_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
)

add_dependencies(${PROJECT_NAME}
  ShaderCompilerHLSL
  InspectorPlugin
  ParticlePlugin
)
