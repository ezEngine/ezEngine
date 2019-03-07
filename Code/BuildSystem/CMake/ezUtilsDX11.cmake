######################################
### ez_link_target_dx11
######################################

function(ez_link_target_dx11 TARGET_NAME)

	find_package (DirectX11)
	target_link_libraries(${TARGET_NAME} 
		# PRIVATE 
		${DirectX11_D3D11_LIBRARIES})

	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
			set (DX11_COPY_DLLS_BIT "x64")
	else ()
			set (DX11_COPY_DLLS_BIT "x86")
	endif ()

	if( ${DirectX11_LIBRARY} MATCHES "/8\\.0/")
			set (DX11_COPY_DLLS_WINSDKVERSION "8.0")
			set (DX11_COPY_DLLS_DLL_VERSION "46")
	elseif( ${DirectX11_LIBRARY} MATCHES "/8\\.1/")
			set (DX11_COPY_DLLS_WINSDKVERSION "8.1")
			set (DX11_COPY_DLLS_DLL_VERSION "47")
	elseif( ${DirectX11_LIBRARY} MATCHES "/10/")
			set (DX11_COPY_DLLS_WINSDKVERSION "10")
			set (DX11_COPY_DLLS_DLL_VERSION "47")
	endif ()

	if (${DX11_COPY_DLLS_WINSDKVERSION})
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"%ProgramFiles(x86)%/Windows Kits/${DX11_COPY_DLLS_WINSDKVERSION}/Redist/D3D/${DX11_COPY_DLLS_BIT}/d3dcompiler_${DX11_COPY_DLLS_DLL_VERSION}.dll"
			$<TARGET_FILE_DIR:${TARGET_NAME}>)
	endif ()

endfunction()

