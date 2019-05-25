ez_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
endif()

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation

  PRIVATE
  mikktspace
)

if (EZ_3RDPARTY_LUA_SUPPORT)

  target_link_libraries(${PROJECT_NAME}
    PUBLIC
    Lua
  )

endif()
