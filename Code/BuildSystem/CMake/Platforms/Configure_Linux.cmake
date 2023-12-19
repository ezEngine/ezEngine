message(STATUS "Configuring Platform: Linux")

# #####################################
# ## General settings
# #####################################
set(EZ_COMPILE_ENGINE_AS_DLL ON CACHE BOOL "Whether to compile the code as a shared libraries (DLL).")
mark_as_advanced(FORCE EZ_COMPILE_ENGINE_AS_DLL)

# #####################################
# ## Experimental Editor support on Linux
# #####################################
set (EZ_EXPERIMENTAL_EDITOR_ON_LINUX OFF CACHE BOOL "Wether or not to build the editor on linux")


macro(ez_pull_platform_properties)

    get_property(EZ_CMAKE_PLATFORM_LINUX GLOBAL PROPERTY EZ_CMAKE_PLATFORM_LINUX)

endmacro()

