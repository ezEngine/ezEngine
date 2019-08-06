
######################################
### ez_check_build_type()
######################################

function(ez_check_build_type)

	# set the default build type

	if (NOT CMAKE_BUILD_TYPE)
		
		set (CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
	
	endif()

endfunction()


######################################
### ez_set_build_flags_msvc(<target>)
######################################

function(ez_set_build_flags_msvc TARGET_NAME)

	#target_compile_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:DEBUG>:${MY_DEBUG_OPTIONS}>")
	
	# enable multi-threaded compilation
	target_compile_options(${TARGET_NAME} PRIVATE "/MP")
	
	# disable RTTI
	target_compile_options(${TARGET_NAME} PRIVATE "/GR-")
	
	# use fast floating point model
	target_compile_options(${TARGET_NAME} PRIVATE "/fp:fast")
	
	# enable floating point exceptions
	# target_compile_options(${TARGET_NAME} PRIVATE "/fp:except")
	
	# enable default exception handling
	target_compile_options(${TARGET_NAME} PRIVATE "/EHsc")
	
	# nothing in UWP headers is standard conform so have to skip this for UWP
	if (NOT CMAKE_SYSTEM_NAME MATCHES "WindowsStore")
		# disable permissive mode
		target_compile_options(${TARGET_NAME} PRIVATE "/permissive-")
	endif ()
	
	# enable standard conform casting behavior - casting results always in rvalue
	target_compile_options(${TARGET_NAME} PRIVATE "/Zc:rvalueCast")
	
	# force the compiler to interpret code as utf8.
	target_compile_options(${TARGET_NAME} PRIVATE "/utf-8")
	
	# /WX: treat warnings as errors
	if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		target_compile_options(${TARGET_NAME} PRIVATE "/WX")
	endif()
	
	if ((CMAKE_SIZEOF_VOID_P EQUAL 4) AND EZ_CMAKE_ARCHITECTURE_X86)
		# enable SSE2 (incompatible with /fp:except)
		target_compile_options(${TARGET_NAME} PRIVATE "/arch:SSE2")
	endif ()
	
	# /Zo: Improved debugging of optimized code
	target_compile_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:RELEASE>:/Zo>")
	target_compile_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:RELWITHDEBINFO>:/Zo>")
	
	# /Ob1: Only consider functions for inlining that are marked with inline or forceinline
	target_compile_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:DEBUG>:/Ob1>")
	
	# /Ox: favor speed for optimizations
	target_compile_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:RELEASE>:/Ox>")
	# /Ob2: Consider all functions for inlining
	target_compile_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:RELEASE>:/Ob2>")
	# /Oi: Replace some functions with intrinsics or other special forms of the function
	target_compile_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:RELEASE>:/Oi>")
	
	# Enable SSE4.1 for Clang on Windows.
	# Todo: In general we should make this configurable. As of writing SSE4.1 is always active for windows builds (independent of the compiler)
	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND EZ_CMAKE_ARCHITECTURE_X86)
		target_compile_options(${TARGET_NAME} PRIVATE "-msse4.1")
	endif()
	
	set (LINKER_FLAGS_DEBUG "")
	
	#Do not remove unreferenced data. Required to make incremental linking work.
	set (LINKER_FLAGS_DEBUG "${LINKER_FLAGS_DEBUG} /OPT:NOREF")
	
	#Do not enable comdat folding in debug. Required to make incremental linking work.
	set (LINKER_FLAGS_DEBUG "${LINKER_FLAGS_DEBUG} /OPT:NOICF")
	
	set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_DEBUG           ${LINKER_FLAGS_DEBUG})
	set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO  ${LINKER_FLAGS_DEBUG})
	
	set (LINKER_FLAGS_RELEASE "")
	
	set (LINKER_FLAGS_RELEASE "${LINKER_FLAGS_RELEASE} /INCREMENTAL:NO")
		
	# Remove unreferenced data (does not work together with incremental build)
	set (LINKER_FLAGS_RELEASE "${LINKER_FLAGS_RELEASE} /OPT:REF")
		
	# Enable comdat folding. Reduces the number of redudant template functions and thus reduces binary size. Makes debugging harder though.
	set (LINKER_FLAGS_RELEASE "${LINKER_FLAGS_RELEASE} /OPT:ICF")	

  	set_target_properties (${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE        ${LINKER_FLAGS_RELEASE})
	set_target_properties (${TARGET_NAME} PROPERTIES LINK_FLAGS_MINSIZEREL     ${LINKER_FLAGS_RELEASE})
  		
endfunction()

######################################
### ez_set_build_flags_clang(<target>)
######################################
				
function(ez_set_build_flags_clang TARGET_NAME)
		
	# Cmake complains that this is not defined on OSX make build.
	#if(EZ_COMPILE_ENGINE_AS_DLL)
	#set (CMAKE_CPP_CREATE_DYNAMIC_LIBRARY ON)
	#else ()
	#set (CMAKE_CPP_CREATE_STATIC_LIBRARY ON)
	#endif ()
	
	if(NOT EZ_CMAKE_PLATFORM_ANDROID)
		target_compile_options(${TARGET_NAME} PRIVATE "-stdlib=libc++")
	endif()
	
	if(EZ_CMAKE_ARCHITECTURE_X86)
		target_compile_options(${TARGET_NAME} PRIVATE "-msse4.1")
	endif()
	
	# Compile for c++17 if compiling a c++ file
	target_compile_options(${TARGET_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=c++17>)
	
	# Disable warning: multi-character character constant
	target_compile_options(${TARGET_NAME} PRIVATE -Wno-multichar)

endfunction()

######################################
### ez_set_build_flags_gcc(<target>)
######################################

function(ez_set_build_flags_gcc TARGET_NAME)

	# Wno-enum-compare removes all annoying enum cast warnings
	target_compile_options(${TARGET_NAME} PRIVATE -fPIC -Wno-enum-compare -mssse3 -mfpmath=sse -gdwarf-3 -pthread)

	# dynamic linking will fail without fPIC (plugins)
	# gdwarf-3 will use the old debug info which is compatible with older gdb versions.
	# these were previously set as CMAKE_C_FLAGS (not CPP)
	target_compile_options(${TARGET_NAME} PRIVATE -fPIC -gdwarf-3)

	target_compile_options(${TARGET_NAME} PRIVATE -msse4.1)
	
	# Disable warning: multi-character character constant
	target_compile_options(${TARGET_NAME} PRIVATE -Wno-multichar)

endfunction()

######################################
### ez_set_build_flags(<target>)
######################################

function(ez_set_build_flags TARGET_NAME)

	ez_pull_compiler_vars()

	set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 17)

	if (EZ_CMAKE_COMPILER_MSVC)

		ez_set_build_flags_msvc(${TARGET_NAME})

	endif()

	if (EZ_CMAKE_COMPILER_CLANG)

		ez_set_build_flags_clang(${TARGET_NAME})

	endif()

	if (EZ_CMAKE_COMPILER_GCC)

		ez_set_build_flags_gcc(${TARGET_NAME})

	endif()

endfunction()
