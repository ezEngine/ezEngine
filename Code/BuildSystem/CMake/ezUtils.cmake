include("ezUtilsPCH")
include("ezUtilsUnityFiles")
include("ezUtilsQt")

######################################
### ez_grep_sources
######################################

function(ez_grep_sources START_FOLDER OUT_FILES)

  file(GLOB_RECURSE SOURCE_FILES_CPP RELATIVE "${START_FOLDER}" "${START_FOLDER}/*.cpp")
  file(GLOB_RECURSE SOURCE_FILES_H RELATIVE "${START_FOLDER}" "${START_FOLDER}/*.h")

  #message(STATUS "Globbed cpp: ${SOURCE_FILES_CPP}")
  #message(STATUS "Globbed h: ${SOURCE_FILES_H}")

  set(${OUT_FILES} "${SOURCE_FILES_CPP};${SOURCE_FILES_H}" PARENT_SCOPE)

endfunction()

######################################
### ez_add_project_files
######################################

function(ez_add_project_files TARGET_NAME ROOT_DIR SUB_FOLDER FILE_LIST)

  foreach(FILE IN ITEMS ${FILE_LIST})
    set(CUR_FILE "${SUB_FOLDER}/${FILE}")
    target_sources(${TARGET_NAME} PRIVATE "${CUR_FILE}")
    source_group(TREE ${ROOT_DIR} FILES ${CUR_FILE})
  endforeach()

endfunction()

######################################
### ez_detect_platform
######################################

function(ez_detect_platform)

	get_property(PREFIX GLOBAL PROPERTY EZ_CMAKE_PLATFORM_PREFIX)
	
	if (PREFIX)
		# has already run before and EZ_CMAKE_PLATFORM_PREFIX is already set
		#message (STATUS "Redundant call to ez_detect_platform()")
		return()
	endif()

	set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_PREFIX "")
	
	set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS_UWP OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS_7 OFF)
	
	set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_POSIX OFF)
	
	set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_OSX OFF)
	
	set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_LINUX OFF)
	
	message (STATUS "CMAKE_SYSTEM_NAME is '${CMAKE_SYSTEM_NAME}'")

	if (CMAKE_SYSTEM_NAME STREQUAL "Windows") # Desktop Windows
	
	  message (STATUS "Platform is Windows (BUILDSYSTEM_PLATFORM_WINDOWS, BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)")
	  
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS ON)
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP ON)
	  
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_PREFIX "Win") 

	  if( ${CMAKE_SYSTEM_VERSION} EQUAL 6.1 )
		set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS_7 ON)
	  endif()

	elseif (CMAKE_SYSTEM_NAME STREQUAL "WindowsStore") # Windows Universal
	  
	  message (STATUS "Platform is Windows Universal (BUILDSYSTEM_PLATFORM_WINDOWS, BUILDSYSTEM_PLATFORM_WINDOWS_UWP)")
	  
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS ON)
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS_UWP ON)
	  
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_PREFIX "WinUWP") 

	elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND CURRENT_OSX_VERSION) # OS X
	  
	  message (STATUS "Platform is OS X (BUILDSYSTEM_PLATFORM_OSX, BUILDSYSTEM_PLATFORM_POSIX)")
	  
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_POSIX ON)
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_OSX ON)
	
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_PREFIX "Osx") 

	elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux") # Linux
	
	  message (STATUS "Platform is Linux (BUILDSYSTEM_PLATFORM_LINUX, BUILDSYSTEM_PLATFORM_POSIX)")
	  
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_POSIX ON)
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_LINUX ON)
	
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_PREFIX "Linux") 

	else ()
	
	  message (FATAL_ERROR "Platform '${CMAKE_SYSTEM_NAME}' is not supported! Please extend ez_detect_platform().")
	  
	endif ()

endfunction()


######################################
### ez_detect_generator
######################################

function(ez_detect_generator)

	get_property(PREFIX GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX)
	
	if (PREFIX)
		# has already run before and EZ_CMAKE_GENERATOR_PREFIX is already set
		#message (STATUS "Redundant call to ez_detect_generator()")
		return()
	endif()
	
	ez_detect_platform()
	get_property(BUILDSYSTEM_PLATFORM_WINDOWS GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS)
	get_property(BUILDSYSTEM_PLATFORM_OSX GLOBAL PROPERTY EZ_CMAKE_PLATFORM_OSX)
	get_property(BUILDSYSTEM_PLATFORM_LINUX GLOBAL PROPERTY EZ_CMAKE_PLATFORM_LINUX)	
	
	set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX "")
	set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION "undefined")
	set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_MSVC OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_XCODE OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_MAKE OFF)
	
	message (STATUS "CMAKE_GENERATOR is '${CMAKE_GENERATOR}'")

	if (BUILDSYSTEM_PLATFORM_WINDOWS) # Supported windows generators
	
	  if (MSVC)
	  
		# Visual Studio (All VS generators define MSVC)
		message (STATUS "Generator is MSVC (BUILDSYSTEM_MSVC)")
		
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_MSVC ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX "Vs")
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION $<CONFIGURATION>)
		
	  else ()
		message (FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Windows! Please extend ez_detect_generator()")
	  endif ()

	elseif (BUILDSYSTEM_PLATFORM_OSX) # Supported OSX generators
	
	  if (CMAKE_GENERATOR STREQUAL "Xcode") # XCODE
	  
		message (STATUS "Buildsystem is Xcode (BUILDSYSTEM_XCODE)")
		
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_XCODE ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX "Xcode")
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION $<CONFIGURATION>)

	  elseif (CMAKE_GENERATOR STREQUAL "Unix Makefiles") # Unix Makefiles (for QtCreator etc.)
	  
		message (STATUS "Buildsystem is Make (BUILDSYSTEM_MAKE)")
		
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_MAKE ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX "Make")
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})

	  else ()
		message (FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on OS X! Please extend ez_detect_generator()")
	  endif ()

	elseif (BUILDSYSTEM_PLATFORM_LINUX)
	
	  if (CMAKE_GENERATOR STREQUAL "Unix Makefiles") # Unix Makefiles (for QtCreator etc.)
		
		message (STATUS "Buildsystem is Make (BUILDSYSTEM_MAKE)")
		
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_MAKE ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX "Make")
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})
		
	  else ()
		message (FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Linux! Please extend ez_detect_generator()")
	  endif ()

	else ()
	  message (FATAL_ERROR "Platform '${CMAKE_SYSTEM_NAME}' has not set up the supported generators. Please extend ez_detect_generator()")
	endif ()

endfunction()


######################################
### ez_detect_compiler
######################################

function(ez_detect_compiler)

	get_property(PREFIX GLOBAL PROPERTY EZ_CMAKE_COMPILER_POSTFIX)
	
	if (PREFIX)
		# has already run before and EZ_CMAKE_COMPILER_POSTFIX is already set
		#message (STATUS "Redundant call to ez_detect_compiler()")
		return()
	endif()

	ez_detect_platform()
	get_property(BUILDSYSTEM_PLATFORM_OSX GLOBAL PROPERTY EZ_CMAKE_PLATFORM_OSX)
	get_property(BUILDSYSTEM_PLATFORM_LINUX GLOBAL PROPERTY EZ_CMAKE_PLATFORM_LINUX)

	ez_detect_generator()
	get_property(GENERATOR_MSVC GLOBAL PROPERTY EZ_CMAKE_GENERATOR_MSVC)
	
	
	set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_POSTFIX "")
	set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_MSVC OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_MSVC_140 OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_MSVC_141 OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_CLANG OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_GCC OFF)

	
	if (GENERATOR_MSVC) # Visual Studio Compiler
	  
	  message (STATUS "Compiler is MSVC (BUILDSYSTEM_COMPILER_MSVC)")
	  message (STATUS "MSVC_VERSION is ${MSVC_VERSION}")

	  set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_MSVC ON)

	  if(MSVC_VERSION GREATER_EQUAL 1910)
	  
		message (STATUS "Compiler is Visual Studio 2017 (BUILDSYSTEM_COMPILER_MSVC_141)")
		set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_MSVC_141 ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_POSTFIX "2017")
		
	  elseif (MSVC_VERSION GREATER_EQUAL 1900)
	  
		message (STATUS "Compiler is Visual Studio 2015 (BUILDSYSTEM_COMPILER_MSVC_140)")
		set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_MSVC_140 ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_POSTFIX "2015")
		
	  else ()
	  
		message (FATAL_ERROR "Compiler for generator '${CMAKE_GENERATOR}' is not supported on MSVC! Please extend ez_detect_compiler()")
		
	  endif ()

	elseif (BUILDSYSTEM_PLATFORM_OSX)
	
	  # Currently all are clang by default.
	  # We should probably make this more idiot-proof in case someone actually changes the compiler to gcc.
	  message (STATUS "Compiler is clang (BUILDSYSTEM_COMPILER_CLANG)")
	  set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_CLANG ON)
	  set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_POSTFIX "Clang")

	elseif (BUILDSYSTEM_PLATFORM_LINUX)
	
	  # Currently all are gcc by default. See OSX comment.
	  message (STATUS "Compiler is gcc (BUILDSYSTEM_COMPILER_GCC)")
	  set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_GCC ON)
	  set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_POSTFIX "Gcc")

	else ()
	  message (FATAL_ERROR "Compiler for generator '${CMAKE_GENERATOR}' is not supported on '${CMAKE_SYSTEM_NAME}'. Please extend ez_detect_compiler()")
	endif ()
	
endfunction()

######################################
### ez_detect_architecture
######################################

function(ez_detect_architecture)

	get_property(PREFIX GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_POSTFIX)
	
	if (PREFIX)
		# has already run before and EZ_CMAKE_ARCHITECTURE_POSTFIX is already set
		#message (STATUS "Redundant call to ez_detect_architecture()")
		return()
	endif()

	set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_POSTFIX "")
	set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_32BIT OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_64BIT OFF)

	if (BUILDSYSTEM_PLATFORM_WINDOWS AND BUILDSYSTEM_COMPILER_MSVC)
	  
	  # Detect 64-bit builds for MSVC.
	  if (CMAKE_CL_64)
	  
		message (STATUS "Platform is 64-Bit (BUILDSYSTEM_PLATFORM_64BIT)")
		set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_64BIT ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_POSTFIX "64")
		
	  else ()
	  
		message (STATUS "Platform is 32-Bit (BUILDSYSTEM_PLATFORM_32BIT)")
		set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_32BIT ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_POSTFIX "32")
		
	  endif ()

	elseif (BUILDSYSTEM_PLATFORM_OSX AND BUILDSYSTEM_COMPILER_CLANG)
	
	  # OS X always has 32/64 bit support in the project files and the user switches on demand.
	  # However, we do not support 32 bit with our current build configuration so we throw an error on 32-bit systems.
	  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	  
		message (STATUS "Platform is 64-Bit (BUILDSYSTEM_PLATFORM_64BIT)")
		set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_64BIT ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_POSTFIX "64")
		
	  else ()
		message (FATAL_ERROR "32-Bit is not supported on OS X!")
	  endif ()

	elseif (BUILDSYSTEM_PLATFORM_LINUX AND BUILDSYSTEM_COMPILER_GCC)
	  
	  # Detect 64-bit builds for Linux, no other way than checking CMAKE_SIZEOF_VOID_P.
	  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	  
		message (STATUS "Platform is 64-Bit (BUILDSYSTEM_PLATFORM_64BIT)")
		set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_64BIT ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_POSTFIX "64")
		
	  else ()
	  
		message (STATUS "Platform is 32-Bit (BUILDSYSTEM_PLATFORM_32BIT)")
		set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_32BIT ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_POSTFIX "32")
		
	  endif ()

	else ()
	  message (FATAL_ERROR "Architecture could not be determined. Please extend CMAKE_GeneralConfig.txt.")
	endif ()	

endfunction()

