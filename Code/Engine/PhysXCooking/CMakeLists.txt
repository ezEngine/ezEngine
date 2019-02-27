#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

# Pass a boolean expression into this macro to build this project only if the expression evaluates to true.
BUILD_CONDITIONAL(BUILDSYSTEM_PLATFORM_WINDOWS)
BUILD_CONDITIONAL(EZ_BUILD_PHYSX)

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

# Add all the libraries that this project needs to link against here
# SET (LINK_LIBRARIES
#  Foundation      # Example: Another library build with cmake
#  OpenGL32.lib    # Example: A library that can be found in any lib path, but was not built with cmake
# )

SET (LINK_LIBRARIES
  Foundation
  Core
  ThirdParty
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

find_package(ezPhysX REQUIRED)

if (EZPHYSX_FOUND)
  target_link_libraries(${PROJECT_NAME} ezPhysX::Cooking)

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezPhysX::Foundation> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezPhysX::Cooking> $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )

endif()

