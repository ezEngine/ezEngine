ez_cmake_init()

ez_build_filter_runtime()

ez_requires_rmlui()

set (PROJECT_NAME "RmlCore")

ez_create_target(LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX)

# override the default IDE folder
ez_set_project_ide_folder(${PROJECT_NAME} "Code/ThirdParty/RmlCore")

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_RMLUI_SUPPORT)

# Don't use standard c++ rtti since it will be disabled in the project
target_compile_definitions(${PROJECT_NAME} PUBLIC RMLUI_USE_CUSTOM_RTTI)

get_property(EZ_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY EZ_SUBMODULE_PREFIX_PATH)
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../../Include")

# Freetype
set(RMLUI_FREETYPE_PATH ${CMAKE_SOURCE_DIR}/${EZ_SUBMODULE_PREFIX_PATH}/Code/ThirdParty/RmlUi/Dependencies/freetype)
target_include_directories(${PROJECT_NAME} PRIVATE "${RMLUI_FREETYPE_PATH}/Include")

if(EZ_CMAKE_PLATFORM_WINDOWS)
  target_compile_definitions(${PROJECT_NAME} PRIVATE RmlCore_EXPORTS)
  
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(RMLUI_FREETYPE_LIB "${RMLUI_FREETYPE_PATH}/win64")
  else()
    set(RMLUI_FREETYPE_LIB "${RMLUI_FREETYPE_PATH}/win32")
  endif()
  
  target_link_libraries(${PROJECT_NAME} PRIVATE "${RMLUI_FREETYPE_LIB}/freetype.lib")

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${RMLUI_FREETYPE_LIB}/freetype.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )
endif()
