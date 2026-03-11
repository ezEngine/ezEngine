set (EZ_3RDPARTY_MINIAUDIO_SUPPORT ON CACHE BOOL "Whether to add support for MiniAudio.")
mark_as_advanced(FORCE EZ_3RDPARTY_MINIAUDIO_SUPPORT)

macro(ez_requires_miniaudio)
	ez_requires(EZ_3RDPARTY_MINIAUDIO_SUPPORT)
endmacro()
