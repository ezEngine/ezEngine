ez_cmake_init()

ez_requires(EZ_CMAKE_PLATFORM_ANDROID)

ez_requires(EZ_3RDPARTY_PERFETTO_SUPPORT)

# Build the Perfetto amalgamated SDK as a static library.

find_package(Threads REQUIRED)

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)
ez_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX NO_WARNINGS_AS_ERRORS)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_PERFETTO_SUPPORT)
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/sdk")
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)

