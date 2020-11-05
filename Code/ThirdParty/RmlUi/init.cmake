### RmlUi
set (EZ_BUILD_RMLUI ON CACHE BOOL "Whether support for RmlUi should be added")

macro(ez_requires_rmlui)
  ez_requires_windows()
  ez_requires(EZ_BUILD_RMLUI)
  if (EZ_CMAKE_PLATFORM_WINDOWS_UWP)
    return()
  endif()
endmacro()