# find the folder into which the Vulkan SDK has been installed

# early out, if this target has been created before
if((TARGET EzVulkan::Loader) AND(TARGET EzVulkan::DXC))
	return()
endif()

set(EZ_VULKAN_DIR $ENV{VULKAN_SDK} CACHE PATH "Directory of the Vulkan SDK")

ez_pull_compiler_and_architecture_vars()
ez_pull_config_vars()

get_property(EZ_SUBMODULE_PREFIX_PATH GLOBAL PROPERTY EZ_SUBMODULE_PREFIX_PATH)

if (COMMAND ez_platformhook_find_vulkan)
	ez_platformhook_find_vulkan()
else()
	message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
endif()
