######################################
### Jolt support
######################################

set (EZ_3RDPARTY_JOLT_SUPPORT ON CACHE BOOL "Whether to add support for the Jolt physics engine.")
mark_as_advanced(FORCE EZ_3RDPARTY_JOLT_SUPPORT)

######################################
### ez_requires_jolt()
######################################

macro(ez_requires_jolt)

	ez_requires(EZ_3RDPARTY_JOLT_SUPPORT)

endmacro()
