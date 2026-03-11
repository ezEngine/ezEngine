set (EZ_3RDPARTY_BC7ENC_RDO_SUPPORT ON CACHE BOOL "Whether to add support for the bc7 compression.")
mark_as_advanced(FORCE EZ_3RDPARTY_BC7ENC_RDO_SUPPORT)

macro(ez_requires_bc7enc_rdo)
	
	ez_requires(EZ_3RDPARTY_BC7ENC_RDO_SUPPORT)

endmacro()