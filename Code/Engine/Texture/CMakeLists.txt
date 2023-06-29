ez_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME} EXCLUDE_FOLDER_FOR_UNITY DirectXTex)

ez_enable_strict_warnings(${PROJECT_NAME})

if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /wd4005)
endif()

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Foundation

  PRIVATE
    stb_image
)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_HAS_TEXTURE)

if(EZ_CMAKE_PLATFORM_LINUX)
  set(THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/${EZ_SUBMODULE_PREFIX_PATH}/Code/ThirdParty")
  target_include_directories(${PROJECT_NAME} PRIVATE "${THIRDPARTY_DIR}/DirectXMath/include" "${THIRDPARTY_DIR}/sal")
endif()

if (EZ_CMAKE_PLATFORM_WINDOWS_UWP)
  # CMake is stupid
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE tinyexr)
endif()
