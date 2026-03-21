#
# CMake file for the masked occlusion culling library
#
set(CMAKE_SUPPRESS_REGENERATION true)
option(USE_AVX512 "Enable experimental AVX-512 support" OFF)
set(CMAKE_CONFIGURATION_TYPES Debug Release)

#
# Lists of all files included in the library
#
set( MOC_AVX512_FILES MaskedOcclusionCullingAVX512.cpp )
set( MOC_AVX2_FILES MaskedOcclusionCullingAVX2.cpp )
set( MOC_SSE_FILES MaskedOcclusionCulling.cpp CullingThreadpool.cpp )
set( MOC_INCLUDE_FILES MaskedOcclusionCulling.h CullingThreadpool.h CompilerSpecific.inl MaskedOcclusionCullingCommon.inl )
set( MOC_FILES ${MOC_AVX512_FILES} ${MOC_AVX2_FILES} ${MOC_SSE_FILES} ${MOC_INCLUDE_FILES} )

#
# Common compiler flags 
#
if(MSVC)
	if(MSVC_VERSION LESS 1900)
		set(CMAKE_CXX_FLAGS "-std=c++11")
	endif()
else()
	set(CMAKE_CXX_FLAGS "-std=c++11 -m64")
endif()

if(MSVC)
#
# Setup compiler flags for AVX-512 files (MSVC)
#

if (USE_AVX512)
	SET_SOURCE_FILES_PROPERTIES( ${MOC_AVX512_FILES} PROPERTIES COMPILE_FLAGS "-DUSE_AVX512=1 /arch:AVX2" )
else()
	SET_SOURCE_FILES_PROPERTIES( ${MOC_AVX512_FILES} PROPERTIES COMPILE_FLAGS "/arch:AVX2" )
endif()

#
# Setup compiler flags for AVX2 files (MSVC)
#
SET_SOURCE_FILES_PROPERTIES( ${MOC_AVX2_FILES} PROPERTIES COMPILE_FLAGS "/arch:AVX2" )

#
# Setup compiler flags for SSE4.1 / SSE2 files (MSVC)
#
if(NOT "${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
	# SSE2 is always enabled on 64-bit architectures, specifying redundant flag produces a compiler warning
	if(MSVC_VERSION LESS 1900)
		SET_SOURCE_FILES_PROPERTIES( ${MOC_SSE_FILES} PROPERTIES COMPILE_FLAGS "/arch:SSE2" )
	endif()
endif()

else()

#
# Setup compiler flags for AVX-512 files
#
if (USE_AVX512)
	SET_SOURCE_FILES_PROPERTIES( ${MOC_AVX512_FILES} PROPERTIES COMPILE_FLAGS "-DUSE_AVX512=1 -mavx512f -mavx512bw -mavx512dq -mavx2 -mfma -msse4.1" )
else()
	SET_SOURCE_FILES_PROPERTIES( ${MOC_AVX512_FILES} PROPERTIES COMPILE_FLAGS "-mavx2 -mfma -msse4.1" )
endif()

#
# Setup compiler flags for AVX2 files
#
SET_SOURCE_FILES_PROPERTIES( ${MOC_AVX2_FILES} PROPERTIES COMPILE_FLAGS "-mavx2 -mfma -msse4.1" )

#
# Setup compiler flags for SSE4.1 / SSE2 files
#
SET_SOURCE_FILES_PROPERTIES( ${MOC_SSE_FILES} PROPERTIES COMPILE_FLAGS "-msse4.1" )

endif()

#
# Create masked occlusion culling library
#
add_library( MaskedOcclusionCulling ${MOC_FILES} )

#
# Add folder to include path
#
target_include_directories(MaskedOcclusionCulling PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
