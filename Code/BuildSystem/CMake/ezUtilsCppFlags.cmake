
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
  	
	if(EZ_ENABLE_COMPILER_STATIC_ANALYSIS)
		target_compile_options(${TARGET_NAME} PRIVATE "/analyze")
	endif()
	
	# Ignore various warnings we are not interrested in
	# 4251 = class 'type' needs to have dll-interface to be used by clients of class 'type2' -> dll export / import issues (mostly with templates)
	# 4345 = behavior change: an object of POD type constructed with an initializer of the form () will be default-initialized
	# 4201 = nonstandard extension used: nameless struct/union
	# 4324 = structure was padded due to alignment specifier
	# 4100 = unreferenced formal parameter
	# 4189 = local variable is initialized but not referenced
	# 4127 = conditional expression is constant
	# 4244 = conversion from 'int 32' to 'int 16', possible loss of data
	# 4245 = signed/unsigned mismatch
	# 4389 = signed/unsigned mismatch
	# 4310 = cast truncates constant value
	target_compile_options(${TARGET_NAME} PRIVATE /wd4251 /wd4345 /wd4201 /wd4324 /wd4100 /wd4189 /wd4127 /wd4244 /wd4245 /wd4389 /wd4310)
	
	# Set Warnings as Errors: Too few/many parameters given for Macro
	target_compile_options(${TARGET_NAME} PRIVATE /we4002 /we4003)
	
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
	
	if(NOT EZ_CMAKE_PLATFORM_ANDROID AND NOT EZ_CMAKE_PLATFORM_WINDOWS)
		target_compile_options(${TARGET_NAME} PRIVATE "-stdlib=libc++")
	endif()
	
	if(EZ_CMAKE_ARCHITECTURE_X86)
		target_compile_options(${TARGET_NAME} PRIVATE "-msse4.1")
	endif()
	
	# Disable warning: multi-character character constant
	target_compile_options(${TARGET_NAME} PRIVATE -Wno-multichar)
	
	if(EZ_CMAKE_PLATFORM_WINDOWS)
		# Disable the warning that clang doesn't support pragma optimize.
		target_compile_options(${TARGET_NAME} PRIVATE -Wno-ignored-pragma-optimize -Wno-pragma-pack)
		# TODO remove
		target_compile_options(${TARGET_NAME} PRIVATE -Wno-dynamic-class-memaccess)
	endif()
	
	if(NOT (CMAKE_CURRENT_SOURCE_DIR MATCHES "Code/ThirdParty"))
		target_compile_options(${TARGET_NAME} PRIVATE -Werror=inconsistent-missing-override -Werror=switch)
	else()
		message(STATUS ">>> Skip strict warnings on ThirdParty target ${TARGET_NAME}")
	endif()
	
	if(EZ_ENABLE_QT_SUPPORT)
		# Ignore any warnings caused by Qt headers
		target_compile_options(${TARGET_NAME} PRIVATE "--system-header-prefix=\"${EZ_QT_DIR}\"")
	endif()
	
	# Ignore any warnings caused by headers inside the ThirdParty directory.
	if(EZ_SUBMODULE_PREFIX_PATH)
		target_compile_options(${TARGET_NAME} PRIVATE "--system-header-prefix=\"${CMAKE_SOURCE_DIR}/${EZ_SUBMODULE_PREFIX_PATH}/Code/ThirdParty\"")
	else()
		target_compile_options(${TARGET_NAME} PRIVATE "--system-header-prefix=\"${CMAKE_SOURCE_DIR}/Code/ThirdParty\"")
	endif()

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

	ez_pull_compiler_and_architecture_vars()

	set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 17)
	
	# There is a bug in the cmake version used by visual studio 2019 which is 3.15.19101501-MSVC_2 that does not correctly pass the c++17 parameter to the compiler. So we need to specify it manually.
	if(ANDROID AND (${CMAKE_VERSION} VERSION_LESS "3.16.0"))
		target_compile_options(${TARGET_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=c++17>)
	endif()

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
