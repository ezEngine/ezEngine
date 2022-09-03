######################################
### Jolt support
######################################

set (EZ_3RDPARTY_KRAUT_SUPPORT ON CACHE BOOL "Whether to add support for procedurally generated trees with Kraut.")
mark_as_advanced(FORCE EZ_3RDPARTY_KRAUT_SUPPORT)

######################################
### ez_requires_kraut()
######################################

macro(ez_requires_kraut)

	ez_requires(EZ_3RDPARTY_KRAUT_SUPPORT)

endmacro()
