ez_cmake_init()

ez_build_filter_runtime()

ez_requires_rmlui()

set (PROJECT_NAME "lunasvg")

ez_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_UNITY NO_EZ_PREFIX)

# override the default IDE folder
ez_set_project_ide_folder(${PROJECT_NAME} "Code/ThirdParty/lunasvg")

target_include_directories(${PROJECT_NAME}
  PRIVATE
  "3rdparty/plutovg"
  "3rdparty/software"

  PUBLIC
  "Include"
)
