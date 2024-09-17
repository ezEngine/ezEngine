# #####################################
# ## ez_requires_renderer()
# #####################################

macro(ez_requires_renderer)
	# PLATFORM-TODO
	if(EZ_BUILD_EXPERIMENTAL_WEBGPU)
		ez_requires_webgpu()
	endif()

	if(EZ_BUILD_EXPERIMENTAL_VULKAN)
		ez_requires_vulkan()
	endif()

	if(EZ_CMAKE_PLATFORM_WINDOWS)
		ez_requires_d3d()
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

		if (TARGET ShaderCompilerVulkan)
			add_dependencies(${TARGET_NAME}
				ShaderCompilerVulkan
			)
		endif()
	endif()

	if(EZ_BUILD_EXPERIMENTAL_WEBGPU)
		target_link_libraries(${TARGET_NAME}
			PRIVATE
			RendererWebGPU
		)

		if (TARGET ShaderCompilerWebGPU)
			add_dependencies(${TARGET_NAME}
				ShaderCompilerWebGPU
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