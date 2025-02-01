ez_cmake_init()

ez_requires(EZ_3RDPARTY_ANGELSCRIPT_SUPPORT)

set(ANGELSCRIPT_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/angelscript.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_array.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_builder.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_bytecode.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_callfunc.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_compiler.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_config.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_configgroup.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_context.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_criticalsection.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_datatype.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_debug.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_generic.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_map.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_memory.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_module.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_objecttype.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_outputbuffer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_parser.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_property.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_restore.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_scriptcode.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_scriptengine.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_scriptfunction.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_scriptnode.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_scriptobject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_string.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_string_util.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_texts.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_thread.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_tokendef.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_tokenizer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_typeinfo.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_variablescope.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/add_on/scriptarray/scriptarray.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/add_on/scriptstdstring/scriptstdstring.h"
)

set(ANGELSCRIPT_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_atomic.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_builder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_bytecode.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_callfunc.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_callfunc_mips.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_callfunc_x86.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_callfunc_x64_gcc.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_callfunc_x64_msvc.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_callfunc_x64_mingw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_compiler.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_configgroup.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_context.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_datatype.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_gc.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_generic.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_globalproperty.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_memory.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_module.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_objecttype.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_outputbuffer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_parser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_restore.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_scriptcode.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_scriptengine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_scriptfunction.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_scriptnode.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_scriptobject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_string.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_string_util.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_thread.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_tokenizer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_typeinfo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/as_variablescope.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/add_on/scriptarray/scriptarray.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/add_on/scriptstdstring/scriptstdstring.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/add_on/scriptstdstring/scriptstdstring_utils.cpp"
)


# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

if(MSVC AND CMAKE_CL_64)
    message(STATUS "AngelScript using MSVC ASM_MASM")

    enable_language(ASM_MASM)
    if(CMAKE_ASM_MASM_COMPILER_WORKS)
        set(ANGELSCRIPT_SOURCE ${ANGELSCRIPT_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/source/as_callfunc_x64_msvc_asm.asm)
    else()
        message(FATAL_ERROR "MSVC x86_64 target requires a working assembler")
    endif()
endif()

if(${ARCHFLAGS} MATCHES "^arm")
    message(STATUS "AngelScript using ARM ASM")

    enable_language(ASM)
    if(CMAKE_ASM_COMPILER_WORKS)
        set(ANGELSCRIPT_SOURCE ${ANGELSCRIPT_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/source/as_callfunc_arm.cpp ${CMAKE_CURRENT_SOURCE_DIR}/source/as_callfunc_arm_gcc.S)
        set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/source/as_callfunc_arm_gcc.S APPEND PROPERTY COMPILE_FLAGS " -Wa,-mimplicit-it=always")
    else()
        message(FATAL_ERROR "ARM target requires a working assembler")
    endif()
endif()

if(${ARCHFLAGS} MATCHES "^aarch64")
    message(STATUS "AngelScript using AARCH64 ASM")

    enable_language(ASM)
    if(CMAKE_ASM_COMPILER_WORKS)
        if(NOT APPLE)
            set(ANGELSCRIPT_SOURCE ${ANGELSCRIPT_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/source/as_callfunc_arm64.cpp ${CMAKE_CURRENT_SOURCE_DIR}/source/as_callfunc_arm64_gcc.S)
        else()
            set(ANGELSCRIPT_SOURCE ${ANGELSCRIPT_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/source/as_callfunc_arm64.cpp ${CMAKE_CURRENT_SOURCE_DIR}/source/as_callfunc_arm64_xcode.S)
        endif()
    else()
        message(FATAL_ERROR "ARM target requires a working assembler")
    endif()
endif()

set(ALL_SOURCES ${ANGELSCRIPT_HEADERS} ${ANGELSCRIPT_SOURCE})

ez_create_target(LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX NO_UNITY MANUAL_SOURCE_FILES "${ALL_SOURCES}")

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_ANGELSCRIPT_SUPPORT)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (EZ_COMPILE_ENGINE_AS_DLL)
  target_compile_definitions(${PROJECT_NAME} PRIVATE ANGELSCRIPT_EXPORT)
  target_compile_definitions(${PROJECT_NAME} PUBLIC ANGELSCRIPT_DLL_LIBRARY_IMPORT)
endif()
