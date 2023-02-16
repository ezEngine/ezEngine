### RmlUi
set (EZ_BUILD_RMLUI ON CACHE BOOL "Whether support for RmlUi should be added")
mark_as_advanced(FORCE EZ_BUILD_RMLUI)

macro(ez_requires_rmlui)
  ez_requires_one_of(EZ_CMAKE_PLATFORM_WINDOWS EZ_CMAKE_PLATFORM_LINUX)
  ez_requires(EZ_BUILD_RMLUI)
  if (EZ_CMAKE_PLATFORM_WINDOWS_UWP)
    return()
  endif()
endmacro()