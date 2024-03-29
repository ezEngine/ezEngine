ez_cmake_init()



ez_requires(EZ_3RDPARTY_OZZ_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX ALL_SYMBOLS_VISIBLE)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_OZZ_SUPPORT)

target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

target_compile_definitions(${PROJECT_NAME} PRIVATE OZZ_BUILD_BASE_LIB)
target_compile_definitions(${PROJECT_NAME} PRIVATE OZZ_BUILD_ANIMATION_LIB)
target_compile_definitions(${PROJECT_NAME} PRIVATE OZZ_BUILD_GEOMETRY_LIB)
target_compile_definitions(${PROJECT_NAME} PRIVATE OZZ_BUILD_ANIMOFFLINE_LIB)
target_compile_definitions(${PROJECT_NAME} PRIVATE OZZ_BUILD_ANIMATIONTOOLS_LIB)
target_compile_definitions(${PROJECT_NAME} PRIVATE OZZ_BUILD_ANIMATIONFBX_LIB)
target_compile_definitions(${PROJECT_NAME} PRIVATE OZZ_BUILD_OPTIONS_LIB)

if(EZ_CMAKE_PLATFORM_LINUX OR EZ_CMAKE_PLATFORM_ANDROID)
  # Right now, running SIMD version on Linux crashes. Fallback to float implementation.
  target_compile_definitions(${PROJECT_NAME} PRIVATE OZZ_BUILD_SIMD_REF)
endif()

if(EZ_CMAKE_COMPILER_MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /wd4661)
endif()

if(EZ_CMAKE_PLATFORM_WINDOWS)
  if (EZ_COMPILE_ENGINE_AS_DLL)
    target_compile_definitions(${PROJECT_NAME} PUBLIC OZZ_USE_DYNAMIC_LINKING)
  endif()
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
