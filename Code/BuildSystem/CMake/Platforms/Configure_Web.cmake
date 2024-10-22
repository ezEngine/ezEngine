include("${CMAKE_CURRENT_LIST_DIR}/Configure_Default.cmake")

message(STATUS "Configuring Platform: Web")

set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WEB ON)
set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_SUPPORTS_WEBGPU ON)

macro(ez_platform_pull_properties)

	get_property(EZ_CMAKE_PLATFORM_WEB GLOBAL PROPERTY EZ_CMAKE_PLATFORM_WEB)

endmacro()

macro(ez_platform_detect_generator)
	if(CMAKE_GENERATOR MATCHES "Ninja" OR CMAKE_GENERATOR MATCHES "Ninja Multi-Config")
		message(STATUS "Buildsystem is Ninja (EZ_CMAKE_GENERATOR_NINJA)")

		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_NINJA ON)
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_PREFIX "Ninja")
		set_property(GLOBAL PROPERTY EZ_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})

	else()
		message(FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Web! Please extend ez_platform_detect_generator()")
	endif()
endmacro()

macro (ez_platformhook_set_build_flags_clang TARGET_NAME)

	target_compile_options(${TARGET_NAME} PRIVATE 
		"-pthread"
		"-mbulk-memory"
		"-matomics"
		"-fno-exceptions"
		
		"-msimd128"
		"-msse4.2"

		# Debug Build
		"$<$<CONFIG:${EZ_BUILDTYPENAME_DEBUG_UPPER}>:-gsource-map>"
		"$<$<CONFIG:${EZ_BUILDTYPENAME_DEBUG_UPPER}>:-g3>"
		
		# Dev Build
		"$<$<CONFIG:${EZ_BUILDTYPENAME_DEV_UPPER}>:-gsource-map>"
		"$<$<CONFIG:${EZ_BUILDTYPENAME_DEV_UPPER}>:-g2>"
		
		# Shipping Build
		"$<$<CONFIG:${EZ_BUILDTYPENAME_SHIPPING_UPPER}>:-g0>"
	)

endmacro()

macro(ez_platformhook_set_application_properties TARGET_NAME)

	target_link_options(${TARGET_NAME} PRIVATE

		# General
		"-sWASM=1"
		"-lembind"
		# "-sMODULARIZE=1" # Doesn't work at startup
		"-sDISABLE_EXCEPTION_CATCHING=1"
		# "-sWASM_BIGINT" # depends on browser support (https://emscripten.org/docs/tools_reference/settings_reference.html?highlight=environment#wasm-bigint)
		"-sFETCH=1"

		# Webpage Template
		"--shell-file" "${CMAKE_SOURCE_DIR}/Code/BuildSystem/Web/em-default.html"

		# Main memory and main thread stack size
		"-sINITIAL_MEMORY=512MB"
		"-sTOTAL_MEMORY=512MB"
		"-sSTACK_SIZE=4MB"
		
		# Threads
		"-pthread"
		"-sUSE_PTHREADS=1"
		"-sPROXY_TO_PTHREAD=1"
		"-sPTHREAD_POOL_SIZE=20"
		"-sDEFAULT_PTHREAD_STACK_SIZE=1MB"

		# WebGPU
		"-sUSE_WEBGPU=1"
		"-sOFFSCREENCANVAS_SUPPORT"

		# Debug Build
		"$<$<CONFIG:${EZ_BUILDTYPENAME_DEBUG_UPPER}>:-sSTACK_OVERFLOW_CHECK=1>"
		"$<$<CONFIG:${EZ_BUILDTYPENAME_DEBUG_UPPER}>:-sASSERTIONS=1>"
		"$<$<CONFIG:${EZ_BUILDTYPENAME_DEBUG_UPPER}>:-g3>"

		# Dev Build
		"$<$<CONFIG:${EZ_BUILDTYPENAME_DEV_UPPER}>:-sASSERTIONS=1>"
		"$<$<CONFIG:${EZ_BUILDTYPENAME_DEV_UPPER}>:-g2>"

		# Shipping Build
		"$<$<CONFIG:${EZ_BUILDTYPENAME_SHIPPING_UPPER}>:-g0>"
	)

endmacro()

macro(ez_platformhook_package_files TARGET_NAME SRC_FOLDER DST_FOLDER)

	target_link_options(${TARGET_NAME} PRIVATE "SHELL: --preload-file ${SRC_FOLDER}@/${DST_FOLDER}")

endmacro()
