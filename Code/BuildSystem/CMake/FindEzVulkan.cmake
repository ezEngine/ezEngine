# find the folder into which the Vulkan SDK has been installed

# early out, if this target has been created before
if(EzVulkan_FOUND)
	return()
endif()

ez_pull_compiler_and_architecture_vars()
ez_pull_config_vars()

get_property(EZ_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY EZ_SUBMODULE_PREFIX_PATH)

if (COMMAND ez_platformhook_find_vulkan)
	ez_platformhook_find_vulkan()
else()
	message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
endif()
