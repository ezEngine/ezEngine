BUILD_CONDITIONAL(EZ_ENABLE_D_SCRIPT AND BUILDSYSTEM_PLATFORM_WINDOWS AND BUILDSYSTEM_PLATFORM_64BIT)

if (EZ_USE_VISUALD)
  set(INCLUDE_FILE "${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_GenVisualDProject.txt")
else()
  set(INCLUDE_FILE "${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_DMake.txt")
endif()

# ScriptHook project
set(PROJECT_TYPE "lib")
set(PROJECT_NAME "ScriptHook")
set(DPROJ_GUID 060B21ED-4D6E-4E30-A862-1AEDE7FC2ABD)
set(DFILES_LIST "${CMAKE_CURRENT_LIST_DIR}/ez/Script/Hook/Hook.d")
SET(LINK_LIBRARIES "")
include(${INCLUDE_FILE})

# DScript Project
set(PROJECT_TYPE "dll")
set(PROJECT_NAME "DScript")
set(DPROJ_GUID CE4F3549-1C49-4AD2-9639-CC96D508FE75)
file(GLOB_RECURSE DFILES_LIST "*.d")
list(REMOVE_ITEM DFILES_LIST "${CMAKE_CURRENT_LIST_DIR}/ez/Script/Hook/Hook.d")
list(REMOVE_ITEM DFILES_LIST "${CMAKE_CURRENT_LIST_DIR}/test.d")
list(REMOVE_ITEM DFILES_LIST "${CMAKE_CURRENT_LIST_DIR}/testEz.d")
SET(LINK_LIBRARIES DGlue Foundation)

find_program(EZ_DMD "dmd.exe" PATHS "$ENV{EZ_DMD_PATH}/windows/bin" NO_DEFAULT_PATH)
get_filename_component(EZ_DMD_DIR "${EZ_DMD}" DIRECTORY)
get_filename_component(EZ_DMD_DIR "${EZ_DMD_DIR}" DIRECTORY) 

SET(DPOST_BUILD_DEBUG 
  "\"${CMAKE_COMMAND}\" -E copy_if_different \"${EZ_DMD_DIR}/bin64/druntime64ds.dll\" \"${OUTPUT_DLL_DEBUG}\""
  "\"${CMAKE_COMMAND}\" -E copy_if_different \"${EZ_DMD_DIR}/bin64/druntime64ds.pdb\" \"${OUTPUT_DLL_DEBUG}\""
  "\"${CMAKE_COMMAND}\" -E copy_if_different \"${EZ_DMD_DIR}/bin64/phobos64ds.dll\" \"${OUTPUT_DLL_DEBUG}\""
  "\"${CMAKE_COMMAND}\" -E copy_if_different \"${EZ_DMD_DIR}/bin64/phobos64ds.pdb\" \"${OUTPUT_DLL_DEBUG}\""
  "\"${CMAKE_COMMAND}\" -E copy_if_different \"${EZ_DMD_DIR}/bin64/libcurl.dll\" \"${OUTPUT_DLL_DEBUG}\""
)
  
foreach(CONF RELEASE MINSIZE RELWITHDEB)
  SET(DPOST_BUILD_${CONF} 
    "\"${CMAKE_COMMAND}\" -E copy_if_different \"${EZ_DMD_DIR}/bin64/druntime64s.dll\" \"${OUTPUT_DLL_${CONF}}\""
    "\"${CMAKE_COMMAND}\" -E copy_if_different \"${EZ_DMD_DIR}/bin64/phobos64s.dll\" \"${OUTPUT_DLL_${CONF}}\""
    "\"${CMAKE_COMMAND}\" -E copy_if_different \"${EZ_DMD_DIR}/bin64/libcurl.dll\" \"${OUTPUT_DLL_${CONF}}\""
  )
endforeach()
include(${INCLUDE_FILE})
