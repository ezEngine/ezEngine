# #####################################
# ## ez_requires_renderer()
# #####################################

macro(ez_requires_renderer)
	# PLATFORM-TODO

	# if we know which backend the user wants, require that
	# otherwise require the platform specific renderer
	# if nothing else is known, this platform doesn't support renderers
	if(EZ_BUILD_EXPERIMENTAL_WEBGPU)
		ez_requires_webgpu()
	elseif(EZ_BUILD_VULKAN)
		ez_requires_vulkan()
	elseif(EZ_BUILD_D3D11)
		ez_requires_d3d()
	else()
		message(STATUS "No renderer available on this platform.")
		return()
	endif()
endmacro()

# #####################################
# ## EZ_DEFAULT_RENDERER
# #####################################

set(EZ_DEFAULT_RENDERER "" CACHE STRING "The renderer to use by default when none is specified on the command line. Leave empty to auto-select (DX11 if built, else Vulkan).")

# #####################################
# ## ez_add_renderers(<target>)
# ## Add all required libraries and dependencies to the given target so it has access to all available renderers.
# #####################################
function(ez_add_renderers TARGET_NAME)
	# PLATFORM-TODO
	if(EZ_BUILD_VULKAN AND EZ_CMAKE_PLATFORM_SUPPORTS_VULKAN)
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

	if(EZ_BUILD_D3D11 AND (EZ_CMAKE_PLATFORM_SUPPORTS_D3D11 OR EZ_BUILD_EXPERIMENTAL_DXVK))
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