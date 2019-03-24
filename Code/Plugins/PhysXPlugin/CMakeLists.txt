ez_cmake_init()

ez_requires_physx()

ez_requires_windows()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
)

find_package(ezPhysX REQUIRED)

if (EZPHYSX_FOUND)
  target_link_libraries(${PROJECT_NAME} PRIVATE ezPhysX::PhysX)
  target_link_libraries(${PROJECT_NAME} PRIVATE ezPhysX::Extensions)
  target_link_libraries(${PROJECT_NAME} PRIVATE ezPhysX::PVD)
  target_link_libraries(${PROJECT_NAME} PRIVATE ezPhysX::Character)

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

