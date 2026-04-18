ez_cmake_init()

ez_requires(EZ_3RDPARTY_MOC_SUPPORT)

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

# Only include library source files, exclude samples, tests, tools and build files
file(GLOB MOC_SOURCE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)

# Remove optional files we don't need
list(FILTER MOC_SOURCE_FILES EXCLUDE REGEX "CullingThreadpool\\.")
list(FILTER MOC_SOURCE_FILES EXCLUDE REGEX "FrameRecorder\\.")

ez_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX NO_UNITY NO_PCH NO_WARNINGS_AS_ERRORS MANUAL_SOURCE_FILES "${MOC_SOURCE_FILES}")

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_MOC_SUPPORT)

# Set per-file SIMD compile flags
if (MSVC)
  set_source_files_properties(MaskedOcclusionCullingAVX512.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
  set_source_files_properties(MaskedOcclusionCullingAVX2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
else()
  set_source_files_properties(MaskedOcclusionCullingAVX512.cpp PROPERTIES COMPILE_FLAGS "-mavx2 -mfma -msse4.1 -mxsave" SKIP_PRECOMPILE_HEADERS ON)
  set_source_files_properties(MaskedOcclusionCullingAVX2.cpp PROPERTIES COMPILE_FLAGS "-mavx2 -mfma -msse4.1 -mxsave" SKIP_PRECOMPILE_HEADERS ON)
  set_source_files_properties(MaskedOcclusionCulling.cpp PROPERTIES COMPILE_FLAGS "-msse4.1 -mxsave" SKIP_PRECOMPILE_HEADERS ON)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
