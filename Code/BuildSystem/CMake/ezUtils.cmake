include(CheckIncludeFileCXX)

include("ezUtilsVars")
include("ezUtilsPCH")
include("ezUtilsUnityFiles")
include("ezUtilsQt")
include("ezUtilsDetect")
include("ezUtilsDX11")
include("ezUtilsNuGet")
include("ezUtilsAssImp")
include("ezUtilsEmbree")
include("ezUtilsCI")
include("ezUtilsCppFlags")
include("ezUtilsAndroid")
include("ezUtilsUWP")
include("ezUtilsTarget")
include("ezUtilsTargetCS")
include("ezUtilsVcpkg")
include("ezUtilsSubmodule")
include("ezUtilsVulkan")
include("ezUtilsDependency")
include("ezUtilsKraut")
include("ezUtilsExternal")

######################################
### ez_set_target_output_dirs(<target> <lib-output-dir> <dll-output-dir>)
######################################

function(ez_set_target_output_dirs TARGET_NAME LIB_OUTPUT_DIR DLL_OUTPUT_DIR)

    ez_pull_all_vars()

    set(SUB_DIR "")

    if(EZ_CMAKE_PLATFORM_WINDOWS_UWP)
        # UWP has deployment problems if all applications output to the same path.
		set(SUB_DIR "/${TARGET_NAME}")
    endif()

	set (OUTPUT_LIB_DEBUG       "${LIB_OUTPUT_DIR}/${EZ_CMAKE_PLATFORM_PREFIX}${EZ_CMAKE_GENERATOR_PREFIX}${EZ_CMAKE_COMPILER_POSTFIX}Debug${EZ_CMAKE_ARCHITECTURE_POSTFIX}${SUB_DIR}")
	set (OUTPUT_LIB_SHIPPING     "${LIB_OUTPUT_DIR}/${EZ_CMAKE_PLATFORM_PREFIX}${EZ_CMAKE_GENERATOR_PREFIX}${EZ_CMAKE_COMPILER_POSTFIX}Shipping${EZ_CMAKE_ARCHITECTURE_POSTFIX}${SUB_DIR}")
	set (OUTPUT_LIB_DEV  "${LIB_OUTPUT_DIR}/${EZ_CMAKE_PLATFORM_PREFIX}${EZ_CMAKE_GENERATOR_PREFIX}${EZ_CMAKE_COMPILER_POSTFIX}Dev${EZ_CMAKE_ARCHITECTURE_POSTFIX}${SUB_DIR}")

	set (OUTPUT_DLL_DEBUG       "${DLL_OUTPUT_DIR}/${EZ_CMAKE_PLATFORM_PREFIX}${EZ_CMAKE_GENERATOR_PREFIX}${EZ_CMAKE_COMPILER_POSTFIX}Debug${EZ_CMAKE_ARCHITECTURE_POSTFIX}${SUB_DIR}")
	set (OUTPUT_DLL_SHIPPING     "${DLL_OUTPUT_DIR}/${EZ_CMAKE_PLATFORM_PREFIX}${EZ_CMAKE_GENERATOR_PREFIX}${EZ_CMAKE_COMPILER_POSTFIX}Shipping${EZ_CMAKE_ARCHITECTURE_POSTFIX}${SUB_DIR}")
	set (OUTPUT_DLL_DEV  "${DLL_OUTPUT_DIR}/${EZ_CMAKE_PLATFORM_PREFIX}${EZ_CMAKE_GENERATOR_PREFIX}${EZ_CMAKE_COMPILER_POSTFIX}Dev${EZ_CMAKE_ARCHITECTURE_POSTFIX}${SUB_DIR}")

    # If we can't use generator expressions the non-generator expression version of the
    # output directory should point to the version matching CMAKE_BUILD_TYPE. This is the case for
    # add_custom_command BYPRODUCTS for example needed by Ninja.
    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set_target_properties(${TARGET_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DLL_DEBUG}"
            LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DLL_DEBUG}"
            ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_LIB_DEBUG}"
        )
	elseif (${CMAKE_BUILD_TYPE} STREQUAL "Shipping")
        set_target_properties(${TARGET_NAME} PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DLL_SHIPPING}"
			LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DLL_SHIPPING}"
			ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_LIB_SHIPPING}"
        )
	elseif (${CMAKE_BUILD_TYPE} STREQUAL "Dev")
        set_target_properties(${TARGET_NAME} PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DLL_DEV}"
			LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DLL_DEV}"
			ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_LIB_DEV}"
        )
    else()
        message(WARNING "Unknown CMAKE_BUILD_TYPE: '${CMAKE_BUILD_TYPE}'. ezEngine cmake scripts support the following 3 build types: 'Debug', 'Dev', 'Shipping'")
    endif()	

    set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DLL_DEBUG}"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DLL_DEBUG}"
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_LIB_DEBUG}"
    )

    set_target_properties(${TARGET_NAME} PROPERTIES
	  RUNTIME_OUTPUT_DIRECTORY_SHIPPING "${OUTPUT_DLL_SHIPPING}"
      LIBRARY_OUTPUT_DIRECTORY_SHIPPING "${OUTPUT_DLL_SHIPPING}"
      ARCHIVE_OUTPUT_DIRECTORY_SHIPPING "${OUTPUT_LIB_SHIPPING}"
    )

    set_target_properties(${TARGET_NAME} PROPERTIES
	    RUNTIME_OUTPUT_DIRECTORY_DEV "${OUTPUT_DLL_DEV}"
        LIBRARY_OUTPUT_DIRECTORY_DEV "${OUTPUT_DLL_DEV}"
        ARCHIVE_OUTPUT_DIRECTORY_DEV "${OUTPUT_LIB_DEV}"
    )

endfunction()

######################################
### ez_set_default_target_output_dirs(<target>)
######################################

function(ez_set_default_target_output_dirs TARGET_NAME)

	set (EZ_OUTPUT_DIRECTORY_LIB "${CMAKE_SOURCE_DIR}/Output/Lib" CACHE PATH "Where to store the compiled .lib files.")
	set (EZ_OUTPUT_DIRECTORY_DLL "${CMAKE_SOURCE_DIR}/Output/Bin" CACHE PATH "Where to store the compiled .dll files.")

    mark_as_advanced(FORCE EZ_OUTPUT_DIRECTORY_LIB)
    mark_as_advanced(FORCE EZ_OUTPUT_DIRECTORY_DLL)

    ez_set_target_output_dirs("${TARGET_NAME}" "${EZ_OUTPUT_DIRECTORY_LIB}" "${EZ_OUTPUT_DIRECTORY_DLL}")

endfunction()


######################################
### ez_write_configuration_txt()
######################################

function(ez_write_configuration_txt)

    # Clear Targets.txt and Tests.txt
    file(WRITE ${CMAKE_BINARY_DIR}/Targets.txt "")
    file(WRITE ${CMAKE_BINARY_DIR}/Tests.txt "")

    ez_pull_all_vars()

	# Write configuration to file, as this is done at configure time we must pin the configuration in place (Dev is used because all build machines use this).
    file(WRITE ${CMAKE_BINARY_DIR}/Configuration.txt "")
	set(CONFIGURATION_DESC "${EZ_CMAKE_PLATFORM_PREFIX}${EZ_CMAKE_GENERATOR_PREFIX}${EZ_CMAKE_COMPILER_POSTFIX}Dev${EZ_CMAKE_ARCHITECTURE_POSTFIX}")
    file(APPEND ${CMAKE_BINARY_DIR}/Configuration.txt ${CONFIGURATION_DESC})

endfunction()


######################################
### ez_add_target_folder_as_include_dir(<target> <path-to-target>)
######################################

function(ez_add_target_folder_as_include_dir TARGET_NAME TARGET_FOLDER)

    get_filename_component(PARENT_DIR ${TARGET_FOLDER} DIRECTORY)

    #target_include_directories(${TARGET_NAME} PRIVATE "${TARGET_FOLDER}")
    target_include_directories(${TARGET_NAME} PUBLIC "${PARENT_DIR}")

endfunction()

######################################
### ez_set_common_target_definitions(<target>)
######################################

function(ez_set_common_target_definitions TARGET_NAME)

    ez_pull_all_vars()

    # set the BUILDSYSTEM_COMPILE_ENGINE_AS_DLL definition
    if (EZ_COMPILE_ENGINE_AS_DLL)
        target_compile_definitions(${TARGET_NAME} PUBLIC BUILDSYSTEM_COMPILE_ENGINE_AS_DLL)
    endif()

    target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_SDKVERSION_MAJOR="${EZ_CMAKE_SDKVERSION_MAJOR}")
    target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_SDKVERSION_MINOR="${EZ_CMAKE_SDKVERSION_MINOR}")
    target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_SDKVERSION_PATCH="${EZ_CMAKE_SDKVERSION_PATCH}")

    # set the BUILDSYSTEM_BUILDTYPE definition
    target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_BUILDTYPE="${EZ_CMAKE_GENERATOR_CONFIGURATION}")
    target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_BUILDTYPE_${EZ_CMAKE_GENERATOR_CONFIGURATION})

    # set the BUILDSYSTEM_BUILDING_XYZ_LIB definition
    string(TOUPPER ${TARGET_NAME} PROJECT_NAME_UPPER)
    target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_BUILDING_${PROJECT_NAME_UPPER}_LIB)
    
    if (EZ_BUILD_EXPERIMENTAL_VULKAN)
        target_compile_definitions(${TARGET_NAME} PRIVATE BUILDSYSTEM_ENABLE_VULKAN_SUPPORT)
    endif()
    
    # on Windows, make sure to use the Unicode API
    target_compile_definitions(${TARGET_NAME} PUBLIC UNICODE _UNICODE)

endfunction()

######################################
### ez_set_project_ide_folder(<target> <path-to-target>)
######################################

function(ez_set_project_ide_folder TARGET_NAME PROJECT_SOURCE_DIR)

    # globally enable sorting targets into folders in IDEs
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    get_filename_component (PARENT_FOLDER ${PROJECT_SOURCE_DIR} PATH)
    get_filename_component (FOLDER_NAME ${PARENT_FOLDER} NAME)
    
    set(IDE_FOLDER "${FOLDER_NAME}")
    
    if (${PROJECT_SOURCE_DIR} MATCHES "${CMAKE_SOURCE_DIR}/")
    
        set(IDE_FOLDER "")
        string(REPLACE "${CMAKE_SOURCE_DIR}/" "" PARENT_FOLDER ${PROJECT_SOURCE_DIR})

        
		get_filename_component (PARENT_FOLDER "${PARENT_FOLDER}" PATH)
		get_filename_component (FOLDER_NAME "${PARENT_FOLDER}" NAME)
		
		get_filename_component (PARENT_FOLDER2 "${PARENT_FOLDER}" PATH)

        while(NOT ${PARENT_FOLDER2} STREQUAL "")

            set(IDE_FOLDER "${FOLDER_NAME}/${IDE_FOLDER}")
            
		get_filename_component (PARENT_FOLDER "${PARENT_FOLDER}" PATH)
		get_filename_component (FOLDER_NAME "${PARENT_FOLDER}" NAME)
            
		get_filename_component (PARENT_FOLDER2 "${PARENT_FOLDER}" PATH)
        
        endwhile()

    endif()

    get_property(EZ_SUBMODULE_MODE GLOBAL PROPERTY EZ_SUBMODULE_MODE)
    
    if(EZ_SUBMODULE_MODE)
        set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "ezEngine/${IDE_FOLDER}")
    else()
        set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER ${IDE_FOLDER})
    endif()

endfunction()

######################################
### ez_add_output_ez_prefix(<target>)
######################################

function(ez_add_output_ez_prefix TARGET_NAME)

    set_target_properties(${TARGET_NAME} PROPERTIES IMPORT_PREFIX "ez")
    set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "ez")

endfunction()

######################################
### ez_set_library_properties(<target>)
######################################

function(ez_set_library_properties TARGET_NAME)

    ez_pull_all_vars()

    if (EZ_CMAKE_PLATFORM_LINUX)
        target_link_libraries (${TARGET_NAME} PRIVATE pthread rt)

        if (EZ_CMAKE_COMPILER_GCC)
            # Workaround for: https://bugs.launchpad.net/ubuntu/+source/gcc-5/+bug/1568899
            target_link_libraries (${TARGET_NAME} PRIVATE -lgcc_s -lgcc)
        endif()
    endif ()

    if (EZ_CMAKE_PLATFORM_OSX OR EZ_CMAKE_PLATFORM_LINUX)
        find_package(X11 REQUIRED)
        find_package(SFML REQUIRED system window)
        target_include_directories (${TARGET_NAME} PRIVATE ${X11_X11_INCLUDE_PATH})
        target_link_libraries (${TARGET_NAME} PRIVATE ${X11_X11_LIB} sfml-window sfml-system)
    endif ()

endfunction()

######################################
### ez_set_application_properties(<target>)
######################################

function(ez_set_application_properties TARGET_NAME)

    ez_pull_all_vars()

    if (EZ_CMAKE_PLATFORM_OSX OR EZ_CMAKE_PLATFORM_LINUX)
        find_package(X11 REQUIRED)
        find_package(SFML REQUIRED system window)
        target_include_directories (${TARGET_NAME} PRIVATE ${X11_X11_INCLUDE_PATH})
        target_link_libraries (${TARGET_NAME} PRIVATE ${X11_X11_LIB} sfml-window sfml-system)
    endif ()

    # We need to link against pthread and rt last or linker errors will occur.
    if (EZ_CMAKE_PLATFORM_LINUX)
        target_link_libraries (${TARGET_NAME} PRIVATE pthread rt)
    endif ()

endfunction()

######################################
### ez_set_natvis_file(<target> <path-to-natvis-file>)
######################################

function(ez_set_natvis_file TARGET_NAME NATVIS_FILE)

    # We need at least visual studio 2015 for this to work
    if ((MSVC_VERSION GREATER 1900) OR (MSVC_VERSION EQUAL 1900))

        target_sources(${TARGET_NAME} PRIVATE ${NATVIS_FILE})

    endif()

endfunction()


######################################
### ez_make_winmain_executable(<target>)
######################################

function(ez_make_winmain_executable TARGET_NAME)

    set_property(TARGET ${TARGET_NAME} PROPERTY WIN32_EXECUTABLE ON)

endfunction()


######################################
### ez_gather_subfolders(<abs-path-to-folder> <out-sub-folders>)
######################################

function(ez_gather_subfolders START_FOLDER RESULT_FOLDERS)

    set(ALL_FILES "")
    set(ALL_DIRS "")

    file(GLOB_RECURSE ALL_FILES RELATIVE "${START_FOLDER}" "${START_FOLDER}/*")

    foreach(FILE ${ALL_FILES})

        get_filename_component(FILE_PATH ${FILE} DIRECTORY)

        list(APPEND ALL_DIRS ${FILE_PATH})

    endforeach()

    list(REMOVE_DUPLICATES ALL_DIRS)

    set(${RESULT_FOLDERS} ${ALL_DIRS} PARENT_SCOPE)

endfunction()


######################################
### ez_glob_source_files(<path-to-folder> <out-files>)
######################################

function(ez_glob_source_files ROOT_DIR RESULT_ALL_SOURCES)

  file(GLOB_RECURSE CPP_FILES "${ROOT_DIR}/*.cpp" "${ROOT_DIR}/*.cc")
  file(GLOB_RECURSE H_FILES "${ROOT_DIR}/*.h" "${ROOT_DIR}/*.hpp" "${ROOT_DIR}/*.inl")
  file(GLOB_RECURSE C_FILES "${ROOT_DIR}/*.c")
  file(GLOB_RECURSE CS_FILES "${ROOT_DIR}/*.cs")

  file(GLOB_RECURSE UI_FILES "${ROOT_DIR}/*.ui")
  file(GLOB_RECURSE QRC_FILES "${ROOT_DIR}/*.qrc")
  file(GLOB_RECURSE DEF_FILES "${ROOT_DIR}/*.def")
  file(GLOB_RECURSE RES_FILES "${ROOT_DIR}/*.ico" "${ROOT_DIR}/*.rc")
  file(GLOB_RECURSE CMAKE_FILES "${ROOT_DIR}/*.cmake" "${ROOT_DIR}/CMakeLists.txt")
  
  set(${RESULT_ALL_SOURCES} ${CPP_FILES} ${H_FILES} ${C_FILES} ${CS_FILES} ${UI_FILES} ${QRC_FILES} ${RES_FILES} ${DEF_FILES} ${CMAKE_FILES} PARENT_SCOPE)

endfunction()

######################################
### ez_add_all_subdirs()
######################################

function(ez_add_all_subdirs)

    # find all cmake files below this directory
    file (GLOB SUB_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt")

    foreach (VAR ${SUB_DIRS})

        get_filename_component (RES ${VAR} DIRECTORY)

        add_subdirectory (${RES})

    endforeach ()

endfunction()

######################################
### ez_cmake_init()
######################################

macro(ez_cmake_init)

    ez_pull_all_vars()

endmacro()

######################################
### ez_requires(<variable>)
######################################

macro(ez_requires)

  if(${ARGC} EQUAL 0)
    return()
  endif()

  set(ALL_ARGS "${ARGN}")

  foreach(arg IN LISTS ALL_ARGS)

    if (NOT ${arg})
      return()
    endif()

  endforeach()

endmacro()

######################################
### ez_requires_windows()
######################################

macro(ez_requires_windows)

    ez_requires(EZ_CMAKE_PLATFORM_WINDOWS)

endmacro()

######################################
### ez_requires_windows_desktop()
######################################

macro(ez_requires_windows_desktop)

    ez_requires(EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP)

endmacro()

######################################
### ez_requires_editor()
######################################

macro(ez_requires_editor)

        ez_requires_qt()
        ez_requires_d3d()

endmacro()

######################################
### ez_add_external_folder(<project-number>)
######################################

function(ez_add_external_projects_folder PROJECT_NUMBER)

    set(CACHE_VAR_NAME "EZ_EXTERNAL_PROJECT${PROJECT_NUMBER}")

    set (${CACHE_VAR_NAME} "" CACHE PATH "A folder outside the ez repository that should be parsed for CMakeLists.txt files to include projects into the ez solution.")

    set(CACHE_VAR_VALUE ${${CACHE_VAR_NAME}})

    if (NOT CACHE_VAR_VALUE)
        return()
    endif()

    set_property(GLOBAL PROPERTY "GATHER_EXTERNAL_PROJECTS" TRUE)
    add_subdirectory(${CACHE_VAR_VALUE} "${CMAKE_BINARY_DIR}/ExternalProject${PROJECT_NUMBER}")
    set_property(GLOBAL PROPERTY "GATHER_EXTERNAL_PROJECTS" FALSE)

endfunction()

######################################
### ez_init_projects()
######################################

function(ez_init_projects)

    # find all init.cmake files below this directory
    file (GLOB_RECURSE INIT_FILES "init.cmake")

    foreach (INIT_FILE ${INIT_FILES})

        message(STATUS "Including '${INIT_FILE}'")
        include("${INIT_FILE}")

    endforeach ()
    
endfunction()

######################################
### ez_finalize_projects()
######################################

function(ez_finalize_projects)

    # find all init.cmake files below this directory
    file (GLOB_RECURSE INIT_FILES "finalize.cmake")

    foreach (INIT_FILE ${INIT_FILES})

        message(STATUS "Including '${INIT_FILE}'")
        include("${INIT_FILE}")

    endforeach ()
    
endfunction()

######################################
### ez_build_filter_init()
######################################

# The build filter is intended to only build a subset of ezEngine. 
# The build filters are configured through cmake files in the 'BuildFilters' directory.
function(ez_build_filter_init)

    file (GLOB_RECURSE FILTER_FILES "${CMAKE_SOURCE_DIR}/*.BuildFilter")

    get_property(EZ_BUILD_FILTER_NAMES GLOBAL PROPERTY EZ_BUILD_FILTER_NAMES)
    
    foreach (VAR ${FILTER_FILES})
        cmake_path(GET VAR STEM FILTER_NAME)
        list(APPEND EZ_BUILD_FILTER_NAMES "${FILTER_NAME}")
    
        message(STATUS "Reading build filter '${FILTER_NAME}'")
        include(${VAR})
    endforeach ()

    list(REMOVE_DUPLICATES EZ_BUILD_FILTER_NAMES)
    set_property(GLOBAL PROPERTY EZ_BUILD_FILTER_NAMES ${EZ_BUILD_FILTER_NAMES})

    set(EZ_BUILD_FILTER "Everything" CACHE STRING "Which projects to include in the solution.")

    get_property(EZ_BUILD_FILTER_NAMES GLOBAL PROPERTY EZ_BUILD_FILTER_NAMES)
    set_property(CACHE EZ_BUILD_FILTER PROPERTY STRINGS ${EZ_BUILD_FILTER_NAMES})
    set_property(GLOBAL PROPERTY EZ_BUILD_FILTER_SELECTED ${EZ_BUILD_FILTER})

endfunction()

######################################
### ez_project_build_filter_index(<PROJECT_NAME> <OUT_INDEX>)
######################################

function(ez_project_build_filter_index PROJECT_NAME OUT_INDEX)

    get_property(SELECTED_FILTER_NAME GLOBAL PROPERTY EZ_BUILD_FILTER_SELECTED)
    set(FILTER_VAR_NAME "EZ_BUILD_FILTER_${SELECTED_FILTER_NAME}")
    get_property(FILTER_PROJECTS GLOBAL PROPERTY ${FILTER_VAR_NAME})

    list(LENGTH FILTER_PROJECTS LIST_LENGTH)

    if (${LIST_LENGTH} GREATER 1)
        list(FIND FILTER_PROJECTS ${PROJECT_NAME} FOUND_INDEX)
        set(${OUT_INDEX} ${FOUND_INDEX} PARENT_SCOPE)
    else()
        set(${OUT_INDEX} 0 PARENT_SCOPE)
    endif()

endfunction()

######################################
### ez_apply_build_filter(<PROJECT_NAME>)
######################################

macro(ez_apply_build_filter PROJECT_NAME)

    ez_project_build_filter_index(${PROJECT_NAME} PROJECT_INDEX)

    if (${PROJECT_INDEX} EQUAL -1)
        get_property(SELECTED_FILTER_NAME GLOBAL PROPERTY EZ_BUILD_FILTER_SELECTED)
        message(STATUS "Project '${PROJECT_NAME}' excluded by build filter '${SELECTED_FILTER_NAME}'.")
        return()
    endif()

endmacro()

######################################
### ez_set_build_types()
######################################

function(ez_set_build_types)

    set(CMAKE_CONFIGURATION_TYPES "Debug;Dev;Shipping" CACHE STRING "" FORCE) 
    set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "EZ build config types" FORCE)

    set(CMAKE_BUILD_TYPE Dev CACHE STRING "The default build type")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES} )

    set(CMAKE_EXE_LINKER_FLAGS_DEV ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_SHIPPING ${CMAKE_EXE_LINKER_FLAGS_RELEASE} CACHE STRING "" FORCE)

    set(CMAKE_SHARED_LINKER_FLAGS_DEV ${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS_SHIPPING ${CMAKE_SHARED_LINKER_FLAGS_RELEASE} CACHE STRING "" FORCE)

    set(CMAKE_STATIC_LINKER_FLAGS_DEV ${CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
    set(CMAKE_STATIC_LINKER_FLAGS_SHIPPING ${CMAKE_STATIC_LINKER_FLAGS_RELEASE} CACHE STRING "" FORCE)

    set(CMAKE_MODULE_LINKER_FLAGS_DEV ${CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
    set(CMAKE_MODULE_LINKER_FLAGS_SHIPPING ${CMAKE_MODULE_LINKER_FLAGS_RELEASE} CACHE STRING "" FORCE)

    set(CMAKE_CXX_FLAGS_DEV ${CMAKE_CXX_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_SHIPPING ${CMAKE_CXX_FLAGS_RELEASE} CACHE STRING "" FORCE)

    set(CMAKE_C_FLAGS_DEV ${CMAKE_C_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS_SHIPPING ${CMAKE_C_FLAGS_RELEASE} CACHE STRING "" FORCE)

    set(CMAKE_CSharp_FLAGS_DEV ${CMAKE_CSharp_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
    set(CMAKE_CSharp_FLAGS_SHIPPING ${CMAKE_CSharp_FLAGS_RELEASE} CACHE STRING "" FORCE)

    set(CMAKE_RC_FLAGS_DEV ${CMAKE_RC_FLAGS_RELWITHDEBINFO} CACHE STRING "" FORCE)
    set(CMAKE_RC_FLAGS_SHIPPING ${CMAKE_RC_FLAGS_RELEASE} CACHE STRING "" FORCE)

    mark_as_advanced(FORCE CMAKE_EXE_LINKER_FLAGS_DEV)
    mark_as_advanced(FORCE CMAKE_EXE_LINKER_FLAGS_SHIPPING)
    mark_as_advanced(FORCE CMAKE_SHARED_LINKER_FLAGS_DEV)
    mark_as_advanced(FORCE CMAKE_SHARED_LINKER_FLAGS_SHIPPING)
    mark_as_advanced(FORCE CMAKE_STATIC_LINKER_FLAGS_DEV)
    mark_as_advanced(FORCE CMAKE_STATIC_LINKER_FLAGS_SHIPPING)
    mark_as_advanced(FORCE CMAKE_MODULE_LINKER_FLAGS_DEV)
    mark_as_advanced(FORCE CMAKE_MODULE_LINKER_FLAGS_SHIPPING)
    mark_as_advanced(FORCE CMAKE_CXX_FLAGS_DEV)
    mark_as_advanced(FORCE CMAKE_CXX_FLAGS_SHIPPING)
    mark_as_advanced(FORCE CMAKE_C_FLAGS_DEV)
    mark_as_advanced(FORCE CMAKE_C_FLAGS_SHIPPING)
    mark_as_advanced(FORCE CMAKE_CSharp_FLAGS_DEV)
    mark_as_advanced(FORCE CMAKE_CSharp_FLAGS_SHIPPING)
    mark_as_advanced(FORCE CMAKE_RC_FLAGS_DEV)
    mark_as_advanced(FORCE CMAKE_RC_FLAGS_SHIPPING)

endfunction()


######################################
### ez_download_and_extract(<url-to-download> <dest-folder-path> <dest-filename-without-extension> <dest-file-extension>)
######################################

function(ez_download_and_extract URL DEST_FOLDER DEST_FILENAME PKG_TYPE)

    set (FULL_FILENAME "${DEST_FILENAME}.${PKG_TYPE}")
    set (PKG_FILE "${DEST_FOLDER}/${FULL_FILENAME}")
    set (EXTRACT_MARKER "${PKG_FILE}.extracted")

    if (EXISTS "${EXTRACT_MARKER}")
        return()
    endif()

    # if the "URL" is actually a file path
    if (NOT "${URL}" MATCHES "http*")
        set(PKG_FILE "${URL}")
    endif()

    if (NOT EXISTS "${PKG_FILE}")

        message(STATUS "Downloading '${FULL_FILENAME}'...")
        file(DOWNLOAD ${URL} "${PKG_FILE}" SHOW_PROGRESS STATUS DOWNLOAD_STATUS)

        list(GET DOWNLOAD_STATUS 0 DOWNLOAD_STATUS_CODE)
        if (NOT DOWNLOAD_STATUS_CODE EQUAL 0)
            message(FATAL_ERROR "Download failed: ${DOWNLOAD_STATUS}")
            return()
        endif()
    endif()

    message(STATUS "Extracting '${FULL_FILENAME}'...")  

    if (${PKG_TYPE} MATCHES "7z")

		execute_process(COMMAND "${CMAKE_SOURCE_DIR}/Data/Tools/Precompiled/7za.exe"
			x "${PKG_FILE}"
			-aoa
            WORKING_DIRECTORY "${DEST_FOLDER}"
            COMMAND_ERROR_IS_FATAL ANY
            RESULT_VARIABLE CMD_STATUS)

    else()

        execute_process(COMMAND ${CMAKE_COMMAND} 
            -E tar -xf "${PKG_FILE}"
            WORKING_DIRECTORY "${DEST_FOLDER}"
            COMMAND_ERROR_IS_FATAL ANY
            RESULT_VARIABLE CMD_STATUS)

    endif()

    if (NOT CMD_STATUS EQUAL 0)
        message(FATAL_ERROR "Extracting package '${FULL_FILENAME}' failed.")
        return()
    endif()

    file(TOUCH ${EXTRACT_MARKER})

endfunction()