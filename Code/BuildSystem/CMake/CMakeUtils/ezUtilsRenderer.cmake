# #####################################
# ## ez_requires_renderer()
# #####################################

macro(ez_requires_renderer)
	# PLATFORM-TODO
	if(EZ_CMAKE_PLATFORM_WINDOWS)
		ez_requires_d3d()
	else()
		ez_requires_vulkan()
	endif()
endmacro()

# #####################################
# ## ez_add_renderers(<target>)
# ## Add all required libraries and dependencies to the given target so it has access to all available renderers.
# #####################################
function(ez_add_renderers TARGET_NAME)
	# PLATFORM-TODO
	if(EZ_BUILD_EXPERIMENTAL_VULKAN)
		target_link_libraries(${TARGET_NAME}
			PRIVATE
			RendererVulkan
		)

		if (TARGET ShaderCompilerDXC)
			add_dependencies(${TARGET_NAME}
				ShaderCompilerDXC
			)
		endif()
	endif()

	if(EZ_CMAKE_PLATFORM_WINDOWS)
		target_link_libraries(${TARGET_NAME}
			PRIVATE
			RendererDX11
		)
		ez_link_target_dx11(${TARGET_NAME})

		if (TARGET ShaderCompilerHLSL)
			add_dependencies(${TARGET_NAME}
				ShaderCompilerHLSL
			)
		endif()
	endif()
endfunction()