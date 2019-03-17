#cmake

ez_all_vars()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
	return()
endif()

if (NOT EZ_BUILD_PHYSX)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

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

