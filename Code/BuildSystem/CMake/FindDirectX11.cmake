#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Find DirectX11 SDK
# Define:
# DirectX11_FOUND
# DirectX11_INCLUDE_DIR
# DirectX11_LIBRARY
# DirectX11_ROOT_DIR

if(WIN32) # The only platform it makes sense to check for DirectX11 SDK
	if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
		set(EZ_CMAKE_WINDOWS_SDK_VERSION ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})
	else()
		set(EZ_CMAKE_WINDOWS_SDK_VERSION ${CMAKE_SYSTEM_VERSION})
		string(REGEX MATCHALL "\\." NUMBER_OF_DOTS "${EZ_CMAKE_WINDOWS_SDK_VERSION}")
		list(LENGTH NUMBER_OF_DOTS NUMBER_OF_DOTS)
		if(NUMBER_OF_DOTS EQUAL 2)
			set(EZ_CMAKE_WINDOWS_SDK_VERSION "${EZ_CMAKE_WINDOWS_SDK_VERSION}.0")
		endif()
	endif()

	include(FindPkgMacros)
	findpkg_begin(DirectX11)

	# Get path, convert backslashes as ${ENV_DXSDK_DIR}
	getenv_path(DXSDK_DIR)
	getenv_path(DIRECTX_HOME)
	getenv_path(DIRECTX_ROOT)
	getenv_path(DIRECTX_BASE)

	# construct search paths
	set(DirectX11_PREFIX_PATH 
	"${DXSDK_DIR}" "${ENV_DXSDK_DIR}"
	"${DIRECTX_HOME}" "${ENV_DIRECTX_HOME}"
	"${DIRECTX_ROOT}" "${ENV_DIRECTX_ROOT}"
	"${DIRECTX_BASE}" "${ENV_DIRECTX_BASE}"
	"C:/apps_x86/Microsoft DirectX SDK*"
	"C:/Program Files (x86)/Microsoft DirectX SDK*"
	"C:/apps/Microsoft DirectX SDK*"
	"C:/Program Files/Microsoft DirectX SDK*"
	"$ENV{ProgramFiles}/Microsoft DirectX SDK*"
	)


	# Windows 8 SDK has custom layout
	set(DirectX11_INC_SEARCH_PATH
	"C:/Program Files (x86)/Windows Kits/10/Include/${EZ_CMAKE_WINDOWS_SDK_VERSION}/shared"
	"C:/Program Files (x86)/Windows Kits/10/Include/${EZ_CMAKE_WINDOWS_SDK_VERSION}/um"
	"C:/Program Files (x86)/Windows Kits/8.1/Include/shared"
	"C:/Program Files (x86)/Windows Kits/8.1/Include/um"
	"C:/Program Files (x86)/Windows Kits/8.0/Include/shared"
	"C:/Program Files (x86)/Windows Kits/8.0/Include/um"

	)
	set(DirectX11_LIB_SEARCH_PATH
  "C:/Program Files (x86)/Windows Kits/10/Lib/${EZ_CMAKE_WINDOWS_SDK_VERSION}/um"
	"C:/Program Files (x86)/Windows Kits/8.1/Lib/winv6.3/um"
	"C:/Program Files (x86)/Windows Kits/8.0/Lib/win8/um"
	)


	# Generate candidate paths from DirectX11_PREFIX_PATH
	create_search_paths(DirectX11)
	
	# redo search if prefix path changed
	clear_if_changed(DirectX11_PREFIX_PATH
		DirectX11_LIBRARY
		DirectX11_INCLUDE_DIR
	)
  
	# dlls are in DirectX11_ROOT_DIR/Developer Runtime/x64|x86
	# lib files are in DirectX11_ROOT_DIR/Lib/x64|x86
	if (EZ_CMAKE_ARCHITECTURE_ARM)
		if(EZ_CMAKE_ARCHITECTURE_64BIT)
			set(DirectX11_LIBPATH_SUFFIX "arm64")
		else()
			set(DirectX11_LIBPATH_SUFFIX "arm")
		endif()
	else()
		if (EZ_CMAKE_ARCHITECTURE_64BIT)
			set (DirectX11_LIBPATH_SUFFIX "x64")
		else ()
			set (DirectX11_LIBPATH_SUFFIX "x86")
		endif ()
	endif()

	# look for D3D11 components
    find_path(DirectX11_INCLUDE_DIR NAMES D3D11Shader.h HINTS ${DirectX11_INC_SEARCH_PATH})
    find_library(DirectX11_DXERR_LIBRARY NAMES DxErr HINTS ${DirectX11_LIB_SEARCH_PATH} PATH_SUFFIXES ${DirectX11_LIBPATH_SUFFIX})
	find_library(DirectX11_DXGUID_LIBRARY NAMES dxguid HINTS ${DirectX11_LIB_SEARCH_PATH} PATH_SUFFIXES ${DirectX11_LIBPATH_SUFFIX})
	find_library(DirectX11_DXGI_LIBRARY NAMES dxgi HINTS ${DirectX11_LIB_SEARCH_PATH} PATH_SUFFIXES ${DirectX11_LIBPATH_SUFFIX})
	find_library(DirectX11_D3DCOMPILER_LIBRARY NAMES d3dcompiler HINTS ${DirectX11_LIB_SEARCH_PATH} PATH_SUFFIXES ${DirectX11_LIBPATH_SUFFIX})

	find_library(DirectX11_LIBRARY NAMES d3d11 HINTS ${DirectX11_LIB_SEARCH_PATH} PATH_SUFFIXES ${DirectX11_LIBPATH_SUFFIX})
    find_library(DirectX11_D3DX11_LIBRARY NAMES d3dx11 HINTS ${DirectX11_LIB_SEARCH_PATH} PATH_SUFFIXES ${DirectX11_LIBPATH_SUFFIX})	
	if (DirectX11_INCLUDE_DIR AND DirectX11_LIBRARY)
	  set(DirectX11_D3D11_FOUND TRUE)
	  set(DirectX11_INCLUDE_DIR ${DirectX11_INCLUDE_DIR})
	  # Need to add 'shared' directory for windows 8 systems:
	  if( ${DirectX11_INCLUDE_DIR} MATCHES "/8\\.0/")
        set(DirectX11_INCLUDE_DIR ${DirectX11_INCLUDE_DIR} "C:/Program Files (x86)/Windows Kits/8.0/Include/shared")
	  elseif( ${DirectX11_INCLUDE_DIR} MATCHES "/8\\.1/")
        set(DirectX11_INCLUDE_DIR ${DirectX11_INCLUDE_DIR} "C:/Program Files (x86)/Windows Kits/8.1/Include/shared")
	  endif ()
	  set(DirectX11_D3D11_LIBRARIES ${DirectX11_D3D11_LIBRARIES}
	    ${DirectX11_LIBRARY}
	    ${DirectX11_DXGI_LIBRARY}
        ${DirectX11_DXGUID_LIBRARY}
        ${DirectX11_D3DCOMPILER_LIBRARY}        	  
      )	
    endif ()
    if (DirectX11_D3DX11_LIBRARY)
        set(DirectX11_D3D11_LIBRARIES ${DirectX11_D3D11_LIBRARIES} ${DirectX11_D3DX11_LIBRARY})
    endif ()
    if (DirectX11_DXERR_LIBRARY)
        set(DirectX11_D3D11_LIBRARIES ${DirectX11_D3D11_LIBRARIES} ${DirectX11_DXERR_LIBRARY})
    endif ()

	findpkg_finish(DirectX11)
	
	set(DirectX11_LIBRARIES 
		${DirectX11_D3D11_LIBRARIES} 
	)
	
	if (OGRE_BUILD_PLATFORM_WINDOWS_PHONE)
		set(DirectX11_FOUND TRUE) 
        set(DirectX11_INCLUDE_DIR "C:/Program Files (x86)/Microsoft Visual Studio 11.0/VC/WPSDK/WP80/include" CACHE STRING "" FORCE)
        set(DirectX11_LIBRARY "C:/Program Files (x86)/Microsoft Visual Studio 11.0/VC/WPSDK/WP80/lib" CACHE STRING "" FORCE)
        set(DirectX11_LIBRARIES ${DirectX11_LIBRARY})
		set(CMAKE_CXX_FLAGS "/EHsc"  CACHE STRING "" FORCE)
	endif ()
  
	mark_as_advanced(DirectX11_INCLUDE_DIR 
					 DirectX11_D3D11_LIBRARIES 
					 DirectX11_D3DX11_LIBRARY
					 DirectX11_DXERR_LIBRARY 
					 DirectX11_DXGUID_LIBRARY
					 DirectX11_DXGI_LIBRARY 
					 DirectX11_D3DCOMPILER_LIBRARY)	
endif(WIN32)