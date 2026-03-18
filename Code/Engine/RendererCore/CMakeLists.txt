ez_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

# The AVX2 software rasterizer needs AVX2+FMA compile flags on non-MSVC compilers.
# MSVC enables these intrinsics without explicit flags.
if (NOT MSVC)
  set_source_files_properties(
    Rasterizer/Thirdparty/Rasterizer.cpp
    Rasterizer/Thirdparty/Occluder.cpp
    PROPERTIES
      COMPILE_OPTIONS "-mavx2;-mfma"
      SKIP_PRECOMPILE_HEADERS ON
  )
endif()

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Core
  RendererFoundation
  Texture
)

if (EZ_3RDPARTY_OZZ_SUPPORT)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ozz
  )
endif()

# needed for shader structure includes
target_include_directories(${PROJECT_NAME} PRIVATE "${EZ_ROOT}/Data/Base")
