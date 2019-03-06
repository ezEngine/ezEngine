#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

if (NOT EZ_BUILD_PHYSX)
	return()
endif()

SET (LINK_LIBRARIES
  Foundation
  Core
  GameEngine
  RendererFoundation
  RendererCore
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

find_package(ezPhysX REQUIRED)

if (EZPHYSX_FOUND)
  target_link_libraries(${PROJECT_NAME} ezPhysX::PhysX)
  target_link_libraries(${PROJECT_NAME} ezPhysX::Extensions)
  target_link_libraries(${PROJECT_NAME} ezPhysX::PVD)
  target_link_libraries(${PROJECT_NAME} ezPhysX::Character)

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezPhysX::nvTools> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezPhysX::Foundation> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezPhysX::Common> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezPhysX::PhysX> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezPhysX::Extensions> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezPhysX::PVD> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezPhysX::Character> $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )

endif()

