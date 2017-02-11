#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

# Pass a boolean expression into this macro to build this project only if the expression evaluates to true.
BUILD_CONDITIONAL(BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)

if (EZ_ENABLE_QT_SUPPORT)
	QT_MODULES(Core Gui Widgets)
	SET (EZ_COPY_QT_DLLS ON)
endif()

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

# Add all the libraries that this project needs to link against here
SET (LINK_LIBRARIES
  Foundation
  Core
  FileservePlugin
)

#if Qt is enabled, link agains the library that contains the UI code
if (EZ_ENABLE_QT_SUPPORT)
	SET (IS_WIN32_APPLICATION true)
	SET (LINK_LIBRARIES ${LINK_LIBRARIES} EditorPluginFileserve)
endif()


include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Application.txt)
