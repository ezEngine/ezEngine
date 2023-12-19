
if(CMAKE_SYSTEM_NAME STREQUAL "Windows") # Desktop Windows

    set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_NAME "Windows")
    set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_PREFIX "Win")
    set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_POSTFIX "Win")

endif()

macro (ez_platform_set_build_flags_clang)
    # Disable the warning that clang doesn't support pragma optimize.
    target_compile_options(${TARGET_NAME} PRIVATE -Wno-ignored-pragma-optimize -Wno-pragma-pack)
endmacro()