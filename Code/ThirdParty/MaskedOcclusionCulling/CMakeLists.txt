ez_cmake_init()

ez_requires(EZ_3RDPARTY_MOC_SUPPORT)

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_pull_compiler_and_architecture_vars()
if (NOT EZ_CMAKE_ARCHITECTURE_X86)
  # GenericMOC only needs the public interface and shared algorithm on non-x86 platforms.
  add_library(${PROJECT_NAME} INTERFACE IMPORTED GLOBAL)
  target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_definitions(${PROJECT_NAME} INTERFACE BUILDSYSTEM_ENABLE_MOC_SUPPORT)
  return()
endif()

# Only include library source files, exclude samples, tests, tools and build files
file(GLOB MOC_SOURCE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)

ez_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX NO_UNITY NO_PCH NO_WARNINGS_AS_ERRORS MANUAL_SOURCE_FILES "${MOC_SOURCE_FILES}")

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_MOC_SUPPORT)

target_compile_definitions(${PROJECT_NAME} PRIVATE EZ_CUSTOM_CHANGES)

# Set per-file SIMD compile flags
if (MSVC)
  set_source_files_properties(MaskedOcclusionCullingAVX512.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX512 /DUSE_AVX512=1")
  set_source_files_properties(MaskedOcclusionCullingAVX2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
else()
  set_source_files_properties(MaskedOcclusionCullingAVX512.cpp PROPERTIES COMPILE_FLAGS "-DUSE_AVX512=1 -mavx512f -mavx512bw -mavx512dq -mavx2 -mfma -msse4.1 -mxsave" SKIP_PRECOMPILE_HEADERS ON)
  set_source_files_properties(MaskedOcclusionCullingAVX2.cpp PROPERTIES COMPILE_FLAGS "-mavx2 -mfma -msse4.1 -mxsave" SKIP_PRECOMPILE_HEADERS ON)
  set_source_files_properties(MaskedOcclusionCulling.cpp PROPERTIES COMPILE_FLAGS "-msse4.1 -mxsave" SKIP_PRECOMPILE_HEADERS ON)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
