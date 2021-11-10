cmake_minimum_required(VERSION 3.20)

set (EZ_SDK_DIR "" CACHE PATH "The ezEngine SDK root directory.")

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

project (!CppProject! VERSION 1.0 LANGUAGES C CXX)

ez_set_build_types()


# Create a modified version of the ezExport.cmake file,
# where the absolute paths to the original locations are replaced
# with the absolute paths to this installation
set(EXP_FILE "${EZ_OUTPUT_DIRECTORY_DLL}/ezExport.cmake")
set(IMP_FILE "${CMAKE_BINARY_DIR}/ezExport.cmake")
set(EXPINFO_FILE "${EZ_OUTPUT_DIRECTORY_DLL}/ezExportInfo.cmake")

# read the file that contains the original paths
include(${EXPINFO_FILE})

# read the ezExport file into a string
file(READ ${EXP_FILE} IMP_CONTENT)

# replace the original paths with our paths
string(REPLACE ${EXPINP_OUTPUT_DIRECTORY_DLL} ${EZ_OUTPUT_DIRECTORY_DLL} IMP_CONTENT "${IMP_CONTENT}")
string(REPLACE ${EXPINP_OUTPUT_DIRECTORY_LIB} ${EZ_OUTPUT_DIRECTORY_LIB} IMP_CONTENT "${IMP_CONTENT}")
string(REPLACE ${EXPINP_SOURCE_DIR} ${EZ_SDK_DIR} IMP_CONTENT "${IMP_CONTENT}")

# write the modified ezExport file to disk
file(WRITE ${IMP_FILE} "${IMP_CONTENT}")

# include the modified file, so that the CMake targets become known
include(${IMP_FILE})


# finally include all the projects
ez_add_all_subdirs()