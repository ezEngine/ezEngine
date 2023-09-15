ez_cmake_init()

ez_requires_rmlui()

set (PROJECT_NAME "lunasvg")

ez_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_UNITY NO_EZ_PREFIX)

# override the default IDE folder
ez_set_project_ide_folder(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/${EZ_SUBMODULE_PREFIX_PATH}/Code/ThirdParty/lunasvg")

target_include_directories(${PROJECT_NAME}
  PRIVATE
  "3rdparty/plutovg"
  "3rdparty/software"

  PUBLIC
  "include"
)

if(EZ_CMAKE_COMPILER_MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /wd4456 /wd4458 /wd4701)
endif()