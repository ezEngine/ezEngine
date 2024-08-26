
if(CMAKE_SYSTEM_NAME STREQUAL "Windows") # Desktop Windows

    set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_NAME "Windows")
    set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_PREFIX "Win")
    set_property(GLOBAL PROPERTY EZ_CMAKE_PLATFORM_POSTFIX "Win")

endif()

