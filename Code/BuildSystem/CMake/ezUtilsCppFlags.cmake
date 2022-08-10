
######################################
### ez_check_build_type()
######################################

function(ez_check_build_type)

	# set the default build type

	if (NOT CMAKE_BUILD_TYPE)
		
		set (CMAKE_BUILD_TYPE Dev CACHE STRING "Choose the type of build, options are: None Debug Dev Shipping." FORCE)
	
	endif()

endfunction()

######################################
### ez_set_build_flags_msvc(<target>)
######################################

function(ez_set_build_flags_msvc TARGET_NAME)

    set(ARG_OPTIONS ENABLE_RTTI NO_WARNINGS_AS_ERRORS NO_CONTROLFLOWGUARD)
    set(ARG_ONEVALUEARGS "")
    set(ARG_MULTIVALUEARGS "")
    cmake_parse_arguments(ARG "${ARG_OPTIONS}" "${ARG_ONEVALUEARGS}" "${ARG_MULTIVALUEARGS}" ${ARGN} )

	#target_compile_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:DEBUG>:${MY_DEBUG_OPTIONS}>")
	
	# enable multi-threaded compilation
	target_compile_options(${TARGET_NAME} PRIVATE "/MP")
	
	# disable RTTI
	if (${ARG_ENABLE_RTTI})
		#message(STATUS "Enabling RTTI for target '${TARGET_NAME}'")
	else()
		target_compile_options(${TARGET_NAME} PRIVATE "/GR-")
	endif()

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

	# set high warning level
	#target_compile_options(${TARGET_NAME} PRIVATE "/W4") # too much work to fix all warnings in ez
	
	# /WX: treat warnings as errors
	if (NOT ${ARG_NO_WARNINGS_AS_ERRORS} AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		target_compile_options(${TARGET_NAME} PRIVATE "/WX")
	endif()
	
	if ((CMAKE_SIZEOF_VOID_P EQUAL 4) AND EZ_CMAKE_ARCHITECTURE_X86)
		# enable SSE2 (incompatible with /fp:except)
		target_compile_options(${TARGET_NAME} PRIVATE "/arch:SSE2")
	endif ()
	
	# /Zo: Improved debugging of optimized code
	target_compile_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:SHIPPING>:/Zo>")
	target_compile_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:DEV>:/Zo>")
	
	# /Ob1: Only consider functions for inlining that are marked with inline or forceinline
	target_compile_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:DEBUG>:/Ob1>")
	
	# /Ox: favor speed for optimizations
	target_compile_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:SHIPPING>:/Ox>")
	# /Ob2: Consider all functions for inlining
	target_compile_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:SHIPPING>:/Ob2>")
	# /Oi: Replace some functions with intrinsics or other special forms of the function
	target_compile_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:SHIPPING>:/Oi>")
	
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
	set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_DEV  ${LINKER_FLAGS_DEBUG})
	
	set (LINKER_FLAGS_SHIPPING "")
	
	set (LINKER_FLAGS_SHIPPING "${LINKER_FLAGS_SHIPPING} /INCREMENTAL:NO")
		
	# Remove unreferenced data (does not work together with incremental build)
	set (LINKER_FLAGS_SHIPPING "${LINKER_FLAGS_SHIPPING} /OPT:REF")
		
	# Enable comdat folding. Reduces the number of redundant template functions and thus reduces binary size. Makes debugging harder though.
	set (LINKER_FLAGS_SHIPPING "${LINKER_FLAGS_SHIPPING} /OPT:ICF")	

  	set_target_properties (${TARGET_NAME} PROPERTIES LINK_FLAGS_SHIPPING        ${LINKER_FLAGS_SHIPPING})
  	
	if(EZ_ENABLE_COMPILER_STATIC_ANALYSIS)
		target_compile_options(${TARGET_NAME} PRIVATE "/analyze")
	endif()
	
	# Ignore various warnings we are not interested in
	
	# 4100 = unreferenced formal parameter *
	# 4127 = conditional expression is constant *
	# 4189 = local variable is initialized but not referenced *
	# 4201 = nonstandard extension used: nameless struct/union *
	# 4245 = signed/unsigned mismatch *
	# 4251 = class 'type' needs to have dll-interface to be used by clients of class 'type2' -> dll export / import issues (mostly with templates) *
	# 4310 = cast truncates constant value *
	# 4324 = structure was padded due to alignment specifier *
	# 4345 = behavior change: an object of POD type constructed with an initializer of the form () will be default-initialized
	# 4389 = signed/unsigned mismatch *
	# 4714 = function 'function' marked as __forceinline not inlined
	# 6326 = Potential comparison of a constant with another constant

	target_compile_options(${TARGET_NAME} PUBLIC /wd4201 /wd4251 /wd4324 /wd4345)
	target_compile_options(${TARGET_NAME} PRIVATE /wd4100 /wd4189 /wd4127 /wd4245 /wd4389 /wd4310 /wd4714 /wd6326)
	
	# Set Warnings as Errors: Too few/many parameters given for Macro
	target_compile_options(${TARGET_NAME} PRIVATE /we4002 /we4003)

	# 4099 = Linker warning "PDB was not found with lib"
	target_link_options(${TARGET_NAME} PRIVATE /ignore:4099)

	# 'nodiscard': attribute is ignored in this syntactic position
	target_compile_options(${TARGET_NAME} PRIVATE /wd5240)

	# ARR specific
	if (${ARG_NO_CONTROLFLOWGUARD})
		#message(STATUS "Disabling ControlFlowGuard for target '${TARGET_NAME}'")
	else()
		target_compile_options(${TARGET_NAME} PRIVATE /guard:cf)
		target_link_options(${TARGET_NAME} PRIVATE /guard:cf)
	endif()

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
	
	if(EZ_CMAKE_PLATFORM_OSX)
		target_compile_options(${TARGET_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
		
		target_link_options(${TARGET_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
	endif()
	
	if(EZ_CMAKE_ARCHITECTURE_X86)
		target_compile_options(${TARGET_NAME} PRIVATE "-msse4.1")
	endif()
	
	if(EZ_CMAKE_PLATFORM_LINUX)
		target_compile_options(${TARGET_NAME} PRIVATE -fPIC)
		
		# Look for the super fast ld compatible linker called "mold". If present we want to use it.
		find_program(MOLD_PATH "mold")
		
		# We want to use the llvm linker lld by default
		# Unless the user has specified a different linker
		get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
		if ("${TARGET_TYPE}" STREQUAL "SHARED_LIBRARY")
			if(NOT ("${CMAKE_EXE_LINKER_FLAGS}" MATCHES "fuse-ld="))
				if (MOLD_PATH)
					target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=${MOLD_PATH}")
				else()
				    target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=lld")
				endif()
			endif()
            
            # Reporting missing symbols at linktime
            target_link_options(${TARGET_NAME} PRIVATE "-Wl,-z,defs")
		elseif("${TARGET_TYPE}" STREQUAL "EXECUTABLE")
			if(NOT ("${CMAKE_SHARED_LINKER_FLAGS}" MATCHES "fuse-ld="))
				if (MOLD_PATH)
					target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=${MOLD_PATH}")
				else()
				    target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=lld")
				endif()
			endif()
            
            # Reporting missing symbols at linktime
            target_link_options(${TARGET_NAME} PRIVATE "-Wl,-z,defs")
		endif()
	endif()
	
	# Disable warning: multi-character character constant
	target_compile_options(${TARGET_NAME} PRIVATE -Wno-multichar)
	
	if(EZ_CMAKE_PLATFORM_WINDOWS)
		# Disable the warning that clang doesn't support pragma optimize.
		target_compile_options(${TARGET_NAME} PRIVATE -Wno-ignored-pragma-optimize -Wno-pragma-pack)
	endif()
	
	if(NOT (CMAKE_CURRENT_SOURCE_DIR MATCHES "Code/ThirdParty"))
		target_compile_options(${TARGET_NAME} PRIVATE -Werror=inconsistent-missing-override -Werror=switch -Werror=uninitialized)
	else()
		# Ignore all warnings in third party code.
		target_compile_options(${TARGET_NAME} PRIVATE -Wno-everything)
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
	target_compile_options(${TARGET_NAME} PRIVATE -fPIC -Wno-enum-compare -gdwarf-3 -pthread)

	if(EZ_CMAKE_ARCHITECTURE_X86)
		target_compile_options(${TARGET_NAME} PRIVATE -mssse3 -mfpmath=sse)
	endif()

	# dynamic linking will fail without fPIC (plugins)
	# gdwarf-3 will use the old debug info which is compatible with older gdb versions.
	# these were previously set as CMAKE_C_FLAGS (not CPP)
	target_compile_options(${TARGET_NAME} PRIVATE -fPIC -gdwarf-3)

	if(EZ_CMAKE_ARCHITECTURE_X86)
		target_compile_options(${TARGET_NAME} PRIVATE -msse4.1)
	endif()
	
	# Disable warning: multi-character character constant
	target_compile_options(${TARGET_NAME} PRIVATE -Wno-multichar)
	
	# Look for the super fast ld compatible linker called "mold". If present we want to use it.
	# GCC can not be told to directly use mold. Instead if we have to look for a symlink called "ld"
	# Which might reside either in /usr/libexec/mold or /usr/local/libexec/mold
	# We can then use gcc's "-B" argument to specify the ld executable path.
	find_program(MOLD_PATH "ld" HINTS "/usr/libexec/mold" "/usr/local/libexec/mold" NO_DEFAULT_PATH)
	
	if (MOLD_PATH)
		get_filename_component(MOLD_DIR ${MOLD_PATH} DIRECTORY)
		# Use the ultra fast mold linker if present and the user didn't specify a different linker manually
		get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
		if ("${TARGET_TYPE}" STREQUAL "SHARED_LIBRARY")
			if(NOT ("${CMAKE_EXE_LINKER_FLAGS}" MATCHES "fuse-ld="))
				target_link_options(${TARGET_NAME} PRIVATE -B ${MOLD_DIR})
			endif()
		elseif("${TARGET_TYPE}" STREQUAL "EXECUTABLE")
			if(NOT ("${CMAKE_SHARED_LINKER_FLAGS}" MATCHES "fuse-ld="))
				target_link_options(${TARGET_NAME} PRIVATE -B ${MOLD_DIR})
			endif()
		endif()
	endif()
    
    # Reporting missing symbols at linktime
    if(("${TARGET_TYPE}" STREQUAL "SHARED_LIBRARY") OR ("${TARGET_TYPE}" STREQUAL "EXECUTABLE"))
        target_link_options(${TARGET_NAME} PRIVATE "-Wl,-z,defs")
    endif()

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

		ez_set_build_flags_msvc(${TARGET_NAME} ${ARGN})

	endif()

	if (EZ_CMAKE_COMPILER_CLANG)

		ez_set_build_flags_clang(${TARGET_NAME} ${ARGN})

	endif()

	if (EZ_CMAKE_COMPILER_GCC)

		ez_set_build_flags_gcc(${TARGET_NAME} ${ARGN})

	endif()

endfunction()
