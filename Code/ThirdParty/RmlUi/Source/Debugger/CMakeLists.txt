ez_cmake_init()
ez_requires_rmlui()

get_property(EZ_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY EZ_SUBMODULE_PREFIX_PATH)

set (PROJECT_NAME "RmlDebugger")

ez_create_target(LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX)

# override the default IDE folder
ez_set_project_ide_folder(${PROJECT_NAME} "${EZ_ROOT}/Code/ThirdParty/RmlDebugger")

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_RMLUI_SUPPORT)
target_compile_definitions(${PROJECT_NAME} PRIVATE RMLUI_DEBUGGER_EXPORTS)

# Don't use standard c++ rtti since it will be disabled in the project
target_compile_definitions(${PROJECT_NAME} PUBLIC RMLUI_CUSTOM_RTTI)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../../Include")

target_link_libraries(${PROJECT_NAME} PUBLIC RmlCore)
