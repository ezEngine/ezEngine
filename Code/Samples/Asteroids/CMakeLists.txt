ez_cmake_init()

ez_requires_d3d()

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

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

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
