ez_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME} EXCLUDE_FOLDER_FOR_UNITY DirectXTex)

ez_enable_strict_warnings(${PROJECT_NAME})

if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /wd4005)
endif()

if(EZ_CMAKE_COMPILER_CLANG)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unknown-pragmas -Wno-unused-const-variable)
endif()

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Foundation

  PRIVATE
    stb_image
)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_HAS_TEXTURE)

if(EZ_CMAKE_PLATFORM_LINUX)
  set(THIRDPARTY_DIR "${EZ_ROOT}/Code/ThirdParty")
  target_include_directories(${PROJECT_NAME} PRIVATE "${THIRDPARTY_DIR}/DirectXMath/include" "${THIRDPARTY_DIR}/sal")
  target_link_libraries(${PROJECT_NAME} PRIVATE bc7enc_rdo)
endif()

if (EZ_CMAKE_PLATFORM_WINDOWS_UWP)
  # CMake is stupid
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE tinyexr)
endif()
