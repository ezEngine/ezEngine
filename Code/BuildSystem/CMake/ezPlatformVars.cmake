# Cmake variables which are platform dependent

# #####################################
# ## General settings
# #####################################
if(EZ_CMAKE_PLATFORM_WINDOWS OR EZ_CMAKE_PLATFORM_LINUX)
	set(EZ_COMPILE_ENGINE_AS_DLL ON CACHE BOOL "Whether to compile the code as a shared libraries (DLL).")
	mark_as_advanced(FORCE EZ_COMPILE_ENGINE_AS_DLL)
else()
	unset(EZ_COMPILE_ENGINE_AS_DLL CACHE)
endif()