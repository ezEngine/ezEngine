ez_cmake_init()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
	return()
endif()

if (NOT EZ_BUILD_PHYSX)
	return()
endif()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Core
)

find_package(ezPhysX REQUIRED)

if (EZPHYSX_FOUND)
  target_link_libraries(${PROJECT_NAME} PRIVATE ezPhysX::Cooking)

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezPhysX::Foundation> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezPhysX::Cooking> $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )

endif()

