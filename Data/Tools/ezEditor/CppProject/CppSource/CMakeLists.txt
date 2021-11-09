cmake_minimum_required(VERSION 3.20)

set (EZ_SDK_DIR "D:/GitHub/ez/ezEngine")
set (EZ_BUILD_DIR "D:/GitHub/Build/vs2019")

# fix output directory
set (EZ_OUTPUT_DIRECTORY_LIB "${EZ_SDK_DIR}/Output/Lib" CACHE PATH "Where to store the compiled .lib files.")
set (EZ_OUTPUT_DIRECTORY_DLL "${EZ_SDK_DIR}/Output/Bin" CACHE PATH "Where to store the compiled .dll files.")

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${EZ_SDK_DIR}/Code/BuildSystem/CMake")

include("ezUtils")

file(RELATIVE_PATH EZ_SUBMODULE_PREFIX_PATH ${CMAKE_SOURCE_DIR} ${EZ_SDK_DIR})
set_property(GLOBAL PROPERTY EZ_SUBMODULE_PREFIX_PATH ${EZ_SUBMODULE_PREFIX_PATH})
if(EZ_SUBMODULE_PREFIX_PATH STREQUAL "")
	set(EZ_SUBMODULE_MODE FALSE)
else()
	set(EZ_SUBMODULE_MODE FALSE)
endif()
set_property(GLOBAL PROPERTY EZ_SUBMODULE_MODE ${EZ_SUBMODULE_MODE})

ez_build_filter_init()

ez_detect_languages()

project (CppProject VERSION 1.0 LANGUAGES C CXX)

ez_set_build_types()

include ("${EZ_BUILD_DIR}/export.cmake")

ez_add_all_subdirs()