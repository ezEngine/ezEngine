ez_cmake_init()

ez_requires_desktop()
ez_requires_vulkan()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

if (EZ_CMAKE_PLATFORM_WINDOWS)
    ez_link_target_dx11(${PROJECT_NAME})
endif()

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  RendererCore
)

ez_link_target_dxc(${PROJECT_NAME})
ez_link_target_vulkan(${PROJECT_NAME})
ez_sources_target_spirv_reflect(${PROJECT_NAME})

if (EZ_CMAKE_PLATFORM_WINDOWS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "_CRT_SECURE_NO_WARNINGS")
endif()

if(EZ_CMAKE_COMPILER_CLANG OR EZ_CMAKE_COMPILER_GCC)
    if(EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP AND EZ_CMAKE_ARCHITECTURE_64BIT)
        set_source_files_properties("${EZ_VULKAN_DIR}/Source/SPIRV-Reflect/spirv_reflect.c" PROPERTIES COMPILE_FLAGS "-Wno-switch")
    elseif(EZ_CMAKE_PLATFORM_LINUX AND EZ_CMAKE_ARCHITECTURE_64BIT)
        set_source_files_properties("${EZ_VULKAN_DIR}/x86_64/include/SPIRV-Reflect/spirv_reflect.c" PROPERTIES COMPILE_FLAGS "-Wno-switch")
    else()
        message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
    endif()
endif()

if(EZ_CMAKE_COMPILER_CLANG)
   target_compile_options(${PROJECT_NAME} PRIVATE -fms-extensions)
endif()
