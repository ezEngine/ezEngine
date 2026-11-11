ez_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME} EXCLUDE_FOLDER_FOR_UNITY Rasterizer/Thirdparty Rasterizer/GenericMOC)

# The AVX2 software rasterizer needs AVX2+FMA compile flags on non-MSVC compilers.
# MSVC enables these intrinsics without explicit flags.
ez_pull_compiler_and_architecture_vars()
if (NOT MSVC AND EZ_CMAKE_ARCHITECTURE_X86)
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

if (EZ_3RDPARTY_MOC_SUPPORT)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    MaskedOcclusionCulling
  )

  # The GenericMOC implementation skips PCH since it redefines SSE intrinsic names
  set_source_files_properties(
    Rasterizer/GenericMOC/MaskedOcclusionCullingGenericMOC.cpp
    PROPERTIES
      SKIP_PRECOMPILE_HEADERS ON
  )
endif()

# needed for shader structure includes
target_include_directories(${PROJECT_NAME} PRIVATE "${EZ_ROOT}/Data/Base")
