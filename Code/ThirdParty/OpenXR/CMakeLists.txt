ez_cmake_init()

ez_requires(EZ_BUILD_OPENXR)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

# Collect all source files
file(GLOB_RECURSE OPENXR_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/common/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/loader/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/external/jsoncpp/*.cpp"
)

file(GLOB OPENXR_SRC_ADDITIONAL
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)
list(APPEND OPENXR_SOURCE ${OPENXR_SRC_ADDITIONAL})

if (EZ_CMAKE_PLATFORM_ANDROID)
    file(GLOB_RECURSE OPENXR_SRC_ANDROID
        "${CMAKE_CURRENT_SOURCE_DIR}/src/external/android-jni-wrappers/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/external/jnipp/*.cpp"
    )
    list(APPEND OPENXR_SOURCE ${OPENXR_SRC_ANDROID})
endif()

set(ALL_SOURCES ${OPENXR_SOURCE})

ez_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX NO_UNITY MANUAL_SOURCE_FILES "${ALL_SOURCES}")

if (EZ_CMAKE_PLATFORM_WINDOWS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC XR_OS_WINDOWS XR_USE_PLATFORM_WIN32 NOMINMAX XR_USE_GRAPHICS_API_D3D11)
elseif (EZ_CMAKE_PLATFORM_OSX)
    target_compile_definitions(${PROJECT_NAME} PUBLIC XR_OS_APPLE XR_USE_GRAPHICS_API_METAL)
elseif (EZ_CMAKE_PLATFORM_LINUX)
    target_compile_definitions(${PROJECT_NAME} PUBLIC XR_OS_LINUX XR_USE_PLATFORM_XLIB XR_USE_GRAPHICS_API_VULKAN)
elseif (EZ_CMAKE_PLATFORM_ANDROID)
    target_compile_definitions(${PROJECT_NAME} PUBLIC XR_OS_ANDROID XR_USE_PLATFORM_ANDROID XR_USE_GRAPHICS_API_VULKAN)
    message(STATUS "Configuring OpenXR for Android")
else()
    message(FATAL_ERROR "Unsupported platform for OpenXR: ${CMAKE_SYSTEM_NAME}")
endif()

# Add the include directories
target_include_directories(${PROJECT_NAME} 
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/common"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/external/jsoncpp/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/external/android-jni-wrappers/"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/external/jnipp/"
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
