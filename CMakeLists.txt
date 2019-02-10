#cmake

cmake_minimum_required(VERSION 3.5)

set (EZ_SOLUTION_NAME "ezEngine" CACHE STRING "The name of the solution")

project (${EZ_SOLUTION_NAME})

#Clear Targets.txt and Tests.txt
file(WRITE ${CMAKE_BINARY_DIR}/Targets.txt "")
file(WRITE ${CMAKE_BINARY_DIR}/Tests.txt "")

#Clear cached qt dirs
set (Qt5Core_DIR "" CACHE PATH "" FORCE)
set (Qt5Gui_DIR "" CACHE PATH "" FORCE)
set (Qt5Widgets_DIR "" CACHE PATH "" FORCE)
set (Qt5Network_DIR "" CACHE PATH "" FORCE)
set (Qt5WinExtras_DIR "" CACHE PATH "" FORCE)

add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/Code)

# sets the Editor project as the default startup project in Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Editor)