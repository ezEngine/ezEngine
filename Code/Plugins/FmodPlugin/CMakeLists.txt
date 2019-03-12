#cmake

set (EZ_BUILD_FMOD OFF CACHE BOOL "Whether support for FMod should be added")

if (NOT EZ_BUILD_FMOD)
	return()
endif()

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
)

find_package(ezFmod REQUIRED)

if (EZFMOD_FOUND)
  target_link_libraries(${PROJECT_NAME} PRIVATE ezFmod::Studio)

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezFmod::Studio> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezFmod::LowLevel> $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )

endif()
