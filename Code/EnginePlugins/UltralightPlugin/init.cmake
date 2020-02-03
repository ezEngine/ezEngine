######################################
### Ultralight support
######################################

set (EZ_BUILD_ULTRALIGHT OFF CACHE BOOL "Whether support for Ultralight should be added")

######################################
### ez_requires_fmod()
######################################

macro(ez_requires_ultralight)

	ez_requires_windows()
	ez_requires(EZ_BUILD_ULTRALIGHT)

endmacro()
