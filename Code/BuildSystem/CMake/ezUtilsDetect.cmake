######################################
### ez_detect_project_name(<out-name>)
######################################

function(ez_detect_project_name OUT_NAME)

	set (DETECTED_NAME "ezEngine")
	
	# unfortunately this has to be known before the PROJECT command, 
	# but platform and compiler settings are only detected by CMake AFTER the project command
	# CMAKE_GENERATOR is the only value available before that, so we have to regex this a bit to
	# generate a useful name
	# thus, only VS solutions currently get nice names

	if (${CMAKE_GENERATOR} MATCHES "Visual Studio")

		set (DETECTED_NAME "ezVs")
		
		if (${CMAKE_GENERATOR} MATCHES "Visual Studio 15")
			set (DETECTED_NAME "${DETECTED_NAME}2017")
		elseif (${CMAKE_GENERATOR} MATCHES "Visual Studio 16")
			set (DETECTED_NAME "${DETECTED_NAME}2019")
		endif()

		if (${CMAKE_GENERATOR} MATCHES "64$")
			set (DETECTED_NAME "${DETECTED_NAME}x64")
		elseif (${CMAKE_GENERATOR} MATCHES "32$")
			set (DETECTED_NAME "${DETECTED_NAME}x32")
		endif()
		
	endif()

	set(${OUT_NAME} "${DETECTED_NAME}" PARENT_SCOPE)

endfunction()

######################################
### ez_detect_platform()
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
	
	  message (STATUS "Platform is Windows (EZ_CMAKE_PLATFORM_WINDOWS, EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP)")
	  
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS ON)
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP ON)
	  
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_PREFIX "Win") 

	  if( ${CMAKE_SYSTEM_VERSION} EQUAL 6.1 )
			set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS_7 ON)
	  endif()

	elseif (CMAKE_SYSTEM_NAME STREQUAL "WindowsStore") # Windows Universal
	  
	  message (STATUS "Platform is Windows Universal (EZ_CMAKE_PLATFORM_WINDOWS, EZ_CMAKE_PLATFORM_WINDOWS_UWP)")
	  
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS ON)
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS_UWP ON)
	  
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_PREFIX "WinUWP") 

	elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND CURRENT_OSX_VERSION) # OS X
	  
	  message (STATUS "Platform is OS X (EZ_CMAKE_PLATFORM_OSX, EZ_CMAKE_PLATFORM_POSIX)")
	  
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_POSIX ON)
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_OSX ON)
	
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_PREFIX "Osx") 

	elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux") # Linux
	
	  message (STATUS "Platform is Linux (EZ_CMAKE_PLATFORM_LINUX, EZ_CMAKE_PLATFORM_POSIX)")
	  
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_POSIX ON)
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_LINUX ON)
	
	  set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_PREFIX "Linux")
	
	elseif (CMAKE_SYSTEM_NAME STREQUAL "Android") # Android
		message (STATUS "Platform is Android (EZ_CMAKE_PLATFORM_ANDROID, EZ_CMAKE_PLATFORM_POSIX)")
		
		set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_POSIX ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_ANDROID ON)
	
		set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_PREFIX "Android")

	else ()
	
	  message (FATAL_ERROR "Platform '${CMAKE_SYSTEM_NAME}' is not supported! Please extend ez_detect_platform().")
	  
	endif ()
	
	get_property(EZ_CMAKE_PLATFORM_WINDOWS GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS)
	if(EZ_CMAKE_PLATFORM_WINDOWS)
		if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
			set(EZ_CMAKE_WINDOWS_SDK_VERSION ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})
		else()
			set(EZ_CMAKE_WINDOWS_SDK_VERSION ${CMAKE_SYSTEM_VERSION})
			string(REGEX MATCHALL "\\." NUMBER_OF_DOTS "${EZ_CMAKE_WINDOWS_SDK_VERSION}")
			list(LENGTH NUMBER_OF_DOTS NUMBER_OF_DOTS)
			if(NUMBER_OF_DOTS EQUAL 2)
				set(EZ_CMAKE_WINDOWS_SDK_VERSION "${EZ_CMAKE_WINDOWS_SDK_VERSION}.0")
			endif()
		endif()
		set_property(GLOBAL PROPERTY EZ_CMAKE_WINDOWS_SDK_VERSION ${EZ_CMAKE_WINDOWS_SDK_VERSION})
	endif()

endfunction()

######################################
### ez_pull_platform_vars()
######################################

macro(ez_pull_platform_vars)

	ez_detect_platform()

	get_property(EZ_CMAKE_PLATFORM_PREFIX GLOBAL PROPERTY EZ_CMAKE_PLATFORM_PREFIX)
	get_property(EZ_CMAKE_PLATFORM_WINDOWS GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS)
	get_property(EZ_CMAKE_PLATFORM_WINDOWS_UWP GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS_UWP)
	get_property(EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP)
	get_property(EZ_CMAKE_PLATFORM_WINDOWS_7 GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WINDOWS_7)
	get_property(EZ_CMAKE_PLATFORM_POSIX GLOBAL PROPERTY EZ_CMAKE_PLATFORM_POSIX)
	get_property(EZ_CMAKE_PLATFORM_OSX GLOBAL PROPERTY EZ_CMAKE_PLATFORM_OSX)
	get_property(EZ_CMAKE_PLATFORM_LINUX GLOBAL PROPERTY EZ_CMAKE_PLATFORM_LINUX)
	get_property(EZ_CMAKE_PLATFORM_ANDROID GLOBAL PROPERTY EZ_CMAKE_PLATFORM_ANDROID)
	
	if(EZ_CMAKE_PLATFORM_WINDOWS)
		get_property(EZ_CMAKE_WINDOWS_SDK_VERSION GLOBAL PROPERTY EZ_CMAKE_WINDOWS_SDK_VERSION)
	endif()

endmacro()

######################################
### ez_detect_generator()
######################################

function(ez_detect_generator)

	get_property(PREFIX GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX)
	
	if (PREFIX)
		# has already run before and EZ_CMAKE_GENERATOR_PREFIX is already set
		#message (STATUS "Redundant call to ez_detect_generator()")
		return()
	endif()
	
	ez_pull_platform_vars()
	
	set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX "")
	set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION "undefined")
	set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_MSVC OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_XCODE OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_MAKE OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_NINJA OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_INSIDE_VS OFF) # if cmake is called through the visual studio open folder workflow
	
		
	message (STATUS "CMAKE_VERSION is '${CMAKE_VERSION}'")
	string(FIND ${CMAKE_VERSION} "MSVC" VERSION_CONTAINS_MSVC)
	if(${VERSION_CONTAINS_MSVC} GREATER -1)
		message(STATUS "cmake was called from visual studio open folder workflow")
		set_property(GLOBAL PROPERTY EZ_CMAKE_INSIDE_VS ON)
	endif()
	
	
	message (STATUS "CMAKE_GENERATOR is '${CMAKE_GENERATOR}'")

	if (EZ_CMAKE_PLATFORM_WINDOWS) # Supported windows generators
	
	  if (MSVC)
	  
			# Visual Studio (All VS generators define MSVC)
			message (STATUS "Generator is MSVC (EZ_CMAKE_GENERATOR_MSVC)")
			
			set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_MSVC ON)
			set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX "Vs")
			set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION $<CONFIGURATION>)
	  elseif(CMAKE_GENERATOR STREQUAL "Ninja") # Ninja makefiles. Only makefile format supported by Visual Studio Open Folder
			message (STATUS "Buildsystem is Ninja (EZ_CMAKE_GENERATOR_NINJA)")
			
			set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_NINJA ON)
			set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX "Ninja")
			set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})
	  else ()
			message (FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Windows! Please extend ez_detect_generator()")
	  endif ()

	elseif (EZ_CMAKE_PLATFORM_OSX) # Supported OSX generators
	
	  if (CMAKE_GENERATOR STREQUAL "Xcode") # XCODE
	  
		message (STATUS "Buildsystem is Xcode (EZ_CMAKE_GENERATOR_XCODE)")
		
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_XCODE ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX "Xcode")
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION $<CONFIGURATION>)

	  elseif (CMAKE_GENERATOR STREQUAL "Unix Makefiles") # Unix Makefiles (for QtCreator etc.)
	  
		message (STATUS "Buildsystem is Make (EZ_CMAKE_GENERATOR_MAKE)")
		
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_MAKE ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX "Make")
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})

	  else ()
		message (FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on OS X! Please extend ez_detect_generator()")
	  endif ()

	elseif (EZ_CMAKE_PLATFORM_LINUX)
	
	  if (CMAKE_GENERATOR STREQUAL "Unix Makefiles") # Unix Makefiles (for QtCreator etc.)
		
		message (STATUS "Buildsystem is Make (EZ_CMAKE_GENERATOR_MAKE)")
		
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_MAKE ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX "Make")
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})
		
	  else ()
		message (FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Linux! Please extend ez_detect_generator()")
	  endif ()

	elseif (EZ_CMAKE_PLATFORM_ANDROID)
		if(CMAKE_GENERATOR STREQUAL "Ninja") # Ninja makefiles. Only makefile format supported by Visual Studio Open Folder
			message (STATUS "Buildsystem is Ninja (EZ_CMAKE_GENERATOR_NINJA)")
			
			set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_NINJA ON)
			set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX "Ninja")
			set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})
			
		else()
			message (FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Android! Please extend ez_detect_generator()")
		endif()
	else ()
	  message (FATAL_ERROR "Platform '${CMAKE_SYSTEM_NAME}' has not set up the supported generators. Please extend ez_detect_generator()")
	endif ()

endfunction()

######################################
### ez_pull_generator_vars()
######################################

macro(ez_pull_generator_vars)

	ez_detect_generator()

	get_property(EZ_CMAKE_GENERATOR_PREFIX GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX)
	get_property(EZ_CMAKE_GENERATOR_CONFIGURATION GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION)
	get_property(EZ_CMAKE_GENERATOR_MSVC GLOBAL PROPERTY EZ_CMAKE_GENERATOR_MSVC)
	get_property(EZ_CMAKE_GENERATOR_XCODE GLOBAL PROPERTY EZ_CMAKE_GENERATOR_XCODE)
	get_property(EZ_CMAKE_GENERATOR_MAKE GLOBAL PROPERTY EZ_CMAKE_GENERATOR_MAKE)
	get_property(EZ_CMAKE_GENERATOR_NINJA GLOBAL PROPERTY EZ_CMAKE_GENERATOR_NINJA)
	get_property(EZ_CMAKE_INSIDE_VS GLOBAL PROPERTY EZ_CMAKE_INSIDE_VS)

endmacro()

######################################
### ez_detect_compiler_and_architecture()
######################################

function(ez_detect_compiler_and_architecture)

	get_property(PREFIX GLOBAL PROPERTY EZ_CMAKE_COMPILER_POSTFIX)
	
	if (PREFIX)
		# has already run before and EZ_CMAKE_COMPILER_POSTFIX is already set
		#message (STATUS "Redundant call to ez_detect_compiler()")
		return()
	endif()

	ez_pull_platform_vars()
	ez_pull_generator_vars()
	get_property(GENERATOR_MSVC GLOBAL PROPERTY EZ_CMAKE_GENERATOR_MSVC)
	
	
	set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_POSTFIX "")
	set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_MSVC OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_MSVC_140 OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_MSVC_141 OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_MSVC_142 OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_CLANG OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_GCC OFF)
	
	set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
	try_compile(COMPILE_RESULT
		${CMAKE_CURRENT_BINARY_DIR}
		${CMAKE_SOURCE_DIR}/${EZ_SUBMODULE_PREFIX_PATH}/Code/BuildSystem/ProbingSrc/ArchitectureDetect.c
		OUTPUT_VARIABLE COMPILE_OUTPUT
	)
	if(NOT COMPILE_RESULT)
		message(FATAL_ERROR "Failed to detect compiler / target architecture. Compiler output: ${COMPILE_OUTPUT}")
	endif()
	
	if(${COMPILE_OUTPUT} MATCHES "ARCH:'([^']*)'")
		set(EZ_DETECTED_ARCH ${CMAKE_MATCH_1})
	else()
		message(FATAL_ERROR "The compile test did not output the architecture. Compiler broken? Compiler output: ${COMPILE_OUTPUT}")
	endif()
	
	if(${COMPILE_OUTPUT} MATCHES "COMPILER:'([^']*)'")
		set(EZ_DETECTED_COMPILER ${CMAKE_MATCH_1})
	else()
		message(FATAL_ERROR "The compile test did not output the compiler. Compiler broken? Compiler output: ${COMPILE_OUTPUT}")
	endif()
	
	if(EZ_DETECTED_COMPILER STREQUAL "msvc")
		if(${COMPILE_OUTPUT} MATCHES "MSC_VER:'([^']*)'")
			set(EZ_DETECTED_MSVC_VER ${CMAKE_MATCH_1})
		else()
			message(FATAL_ERROR "The compile test did not output the MSC_VER. Compiler broken? Compiler output: ${COMPILE_OUTPUT}")
		endif()
	endif()
	
	if(EZ_DETECTED_COMPILER STREQUAL "msvc") # Visual Studio Compiler
	  message (STATUS "Compiler is MSVC (EZ_CMAKE_COMPILER_MSVC) version ${EZ_DETECTED_MSVC_VER}")
		
	  set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_MSVC ON)

      if(EZ_DETECTED_MSVC_VER GREATER_EQUAL 1920)
	  
		message (STATUS "Compiler is Visual Studio 2019 (EZ_CMAKE_COMPILER_MSVC_142)")
		set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_MSVC_142 ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_POSTFIX "2019")
	  
	  elseif(EZ_DETECTED_MSVC_VER GREATER_EQUAL 1910)
	  
		message (STATUS "Compiler is Visual Studio 2017 (EZ_CMAKE_COMPILER_MSVC_141)")
		set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_MSVC_141 ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_POSTFIX "2017")
		
	  elseif (MSVC_VERSION GREATER_EQUAL 1900)
	  
		message (STATUS "Compiler is Visual Studio 2015 (EZ_CMAKE_COMPILER_MSVC_140)")
		set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_MSVC_140 ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_POSTFIX "2015")
		
	  else ()
	  
		message (FATAL_ERROR "Compiler for generator '${CMAKE_GENERATOR}' is not supported on MSVC! Please extend ez_detect_compiler()")
	  endif ()
	  
	elseif(EZ_DETECTED_COMPILER STREQUAL "clang")
	
	  message (STATUS "Compiler is clang (EZ_CMAKE_COMPILER_CLANG)")
	  set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_CLANG ON)
	  set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_POSTFIX "Clang")
	  
	elseif(EZ_DETECTED_COMPILER STREQUAL "gcc")
	
	  message (STATUS "Compiler is gcc (EZ_CMAKE_COMPILER_GCC)")
	  set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_GCC ON)
	  set_property(GLOBAL PROPERTY EZ_CMAKE_COMPILER_POSTFIX "Gcc")
	  
	else()
	
	  message(FATAL_ERROR "Unhandled compiler ${EZ_DETECTED_COMPILER}")
	  
	endif()

	set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_POSTFIX "")
	set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_32BIT OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_64BIT OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_X86 OFF)
	set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_ARM OFF)
	
	if(EZ_DETECTED_ARCH STREQUAL "x86")
	
	  message (STATUS "Platform is X86 (EZ_CMAKE_ARCHITECTURE_X86)")
	  set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_X86 ON)
	  
	  message (STATUS "Platform is 32-Bit (EZ_CMAKE_ARCHITECTURE_32BIT)")
	  set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_32BIT ON)
	  
	elseif(EZ_DETECTED_ARCH STREQUAL "x64")
	
	  message (STATUS "Platform is X86 (EZ_CMAKE_ARCHITECTURE_X86)")
	  set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_X86 ON)
	  
	  message (STATUS "Platform is 64-Bit (EZ_CMAKE_ARCHITECTURE_64BIT)")
	  set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_64BIT ON)
	  
	elseif(EZ_DETECTED_ARCH STREQUAL "arm32")
	
	  message (STATUS "Platform is ARM (EZ_CMAKE_ARCHITECTURE_ARM)")
      set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_ARM ON)
	  
	  message (STATUS "Platform is 32-Bit (EZ_CMAKE_ARCHITECTURE_32BIT)")
	  set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_32BIT ON)
	  
	elseif(EZ_DETECTED_ARCH STREQUAL "arm64")
	
	  message (STATUS "Platform is ARM (EZ_CMAKE_ARCHITECTURE_ARM)")
      set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_ARM ON)
	  
	  message (STATUS "Platform is 64-Bit (EZ_CMAKE_ARCHITECTURE_64BIT)")
	  set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_64BIT ON)
	  
	else()
	  message(FATAL_ERROR "Unhandled target architecture ${EZ_DETECTED_ARCH}")
	endif ()
	
	get_property(EZ_CMAKE_ARCHITECTURE_32BIT GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_32BIT)
	get_property(EZ_CMAKE_ARCHITECTURE_ARM GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_ARM)
	
	if(EZ_CMAKE_ARCHITECTURE_ARM)
		if(EZ_CMAKE_ARCHITECTURE_32BIT)
			set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_POSTFIX "Arm32")
		else()
			set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_POSTFIX "Arm64")
		endif()
	else()
		if(EZ_CMAKE_ARCHITECTURE_32BIT)
			set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_POSTFIX "32")
		else()
			set_property(GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_POSTFIX "64")
		endif()
	endif()
	
endfunction()

######################################
### ez_pull_compiler_vars()
######################################

macro(ez_pull_compiler_and_architecture_vars)

	ez_detect_compiler_and_architecture()

	get_property(EZ_CMAKE_COMPILER_POSTFIX GLOBAL PROPERTY EZ_CMAKE_COMPILER_POSTFIX)
	get_property(EZ_CMAKE_COMPILER_MSVC GLOBAL PROPERTY EZ_CMAKE_COMPILER_MSVC)
	get_property(EZ_CMAKE_COMPILER_MSVC_140 GLOBAL PROPERTY EZ_CMAKE_COMPILER_MSVC_140)
	get_property(EZ_CMAKE_COMPILER_MSVC_141 GLOBAL PROPERTY EZ_CMAKE_COMPILER_MSVC_141)
	get_property(EZ_CMAKE_COMPILER_MSVC_142 GLOBAL PROPERTY EZ_CMAKE_COMPILER_MSVC_142)
	get_property(EZ_CMAKE_COMPILER_CLANG GLOBAL PROPERTY EZ_CMAKE_COMPILER_CLANG)
	get_property(EZ_CMAKE_COMPILER_GCC GLOBAL PROPERTY EZ_CMAKE_COMPILER_GCC)
	
	get_property(EZ_CMAKE_ARCHITECTURE_POSTFIX GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_POSTFIX)
	get_property(EZ_CMAKE_ARCHITECTURE_32BIT GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_32BIT)
	get_property(EZ_CMAKE_ARCHITECTURE_64BIT GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_64BIT)
	get_property(EZ_CMAKE_ARCHITECTURE_X86 GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_X86)
	get_property(EZ_CMAKE_ARCHITECTURE_ARM GLOBAL PROPERTY EZ_CMAKE_ARCHITECTURE_ARM)

endmacro()

######################################
### ez_pull_all_vars()
######################################

macro(ez_pull_all_vars)

	get_property(EZ_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY EZ_SUBMODULE_PREFIX_PATH)

	ez_pull_compiler_and_architecture_vars()
	ez_pull_generator_vars()
	ez_pull_platform_vars()

endmacro()