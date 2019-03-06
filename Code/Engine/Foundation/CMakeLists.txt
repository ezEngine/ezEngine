#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

# Add all the libraries that this project needs to link against here
SET (LINK_LIBRARIES
  ThirdParty
 )

SET(NATVIS_FILE "${CMAKE_SOURCE_DIR}/Utilities/Visual Studio Visualizer/ezEngine.natvis")

if (MSVC)
  SET (LINK_LIBRARIES
    ${LINK_LIBRARIES}
    Rpcrt4.lib
  )
endif()

if (BUILDSYSTEM_PLATFORM_LINUX)
  SET (LINK_LIBRARIES
    ${LINK_LIBRARIES}
    uuid
  )
endif()

if (CURRENT_OSX_VERSION)
  FIND_LIBRARY(CORESERVICES_LIBRARY CoreServices )
  FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation )

  MARK_AS_ADVANCED (CORESERVICES_LIBRARY
                    COREFOUNDATION_LIBRARY)

  SET (LINK_LIBRARIES
    ${LINK_LIBRARIES}
    ${CORESERVICES_LIBRARY}
    ${COREFOUNDATION_LIBRARY}
  )
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

####################################################
## UserConfig header settings

set (EZ_FOUNDATION_IGNORE_USERCONFIG_HEADER OFF CACHE BOOL "When disabled certain compile settings need to be configured through the UserConfig.h file in ezFoundation. When enabled those settings can be done from CMake. Do not enable this when you want to build ezEngine with CMake but then use that library in another project, as the settings in UserConfig.h and the pre-built library will differ.")

mark_as_advanced(FORCE EZ_FOUNDATION_IGNORE_USERCONFIG_HEADER)

if (EZ_FOUNDATION_IGNORE_USERCONFIG_HEADER)

  target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_IGNORE_USERCONFIG_HEADER)
  
  set (EZ_USERCONFIG_USE_PROFILING ON CACHE BOOL "Whether the code for profiling should be compiled in -> #define EZ_USE_PROFILING EZ_ON")
  mark_as_advanced(FORCE EZ_USERCONFIG_USE_PROFILING)
  
  set (EZ_USERCONFIG_COMPILE_FOR_DEVELOPMENT ON CACHE BOOL "Enables various debug checks even in release builds -> #define EZ_COMPILE_FOR_DEVELOPMENT EZ_ON")
  mark_as_advanced(FORCE EZ_USERCONFIG_COMPILE_FOR_DEVELOPMENT)
  
  set (EZ_USERCONFIG_USE_ALLOCATION_STACK_TRACING ON CACHE BOOL "Enables stack tracing for all allocations for easier memory leak detection -> #define EZ_USE_ALLOCATION_STACK_TRACING EZ_ON")
  mark_as_advanced(FORCE EZ_USERCONFIG_USE_ALLOCATION_STACK_TRACING)

  if (EZ_USERCONFIG_USE_PROFILING)
	target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_USE_PROFILING)
  endif()

  if (EZ_USERCONFIG_COMPILE_FOR_DEVELOPMENT)
	target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_COMPILE_FOR_DEVELOPMENT)
  endif()

  if (EZ_USERCONFIG_USE_ALLOCATION_STACK_TRACING)
	target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_USE_ALLOCATION_STACK_TRACING)
  endif()

else()

  unset(EZ_USERCONFIG_USE_PROFILING CACHE)
  unset(EZ_USERCONFIG_COMPILE_FOR_DEVELOPMENT CACHE)
  unset(EZ_USERCONFIG_USE_ALLOCATION_STACK_TRACING CACHE)

endif()





