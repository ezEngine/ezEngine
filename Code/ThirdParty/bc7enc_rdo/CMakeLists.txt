project(bc7enc)

cmake_minimum_required(VERSION 2.8)
option(BUILD_X64 "build 64-bit" TRUE)
option(SUPPORT_BC7E "support BC7E (requires ispc)" FALSE)

message("Initial BUILD_X64=${BUILD_X64}")
message("Initial CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
message("Initial SUPPORT_BC7E=${SUPPORT_BC7E}")

if( NOT CMAKE_BUILD_TYPE )
  set( CMAKE_BUILD_TYPE Release )
endif( NOT CMAKE_BUILD_TYPE )

message( ${PROJECT_NAME} " build type: " ${CMAKE_BUILD_TYPE} )

if (BUILD_X64)
	message("Building 64-bit")
else()
	message("Building 32-bit")
endif(BUILD_X64)

if (NOT MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -fopenmp")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -fopenmp")

	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fopenmp")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fopenmp")
else()
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /openmp /W4")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /openmp /W4")

	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /openmp /W4")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /openmp /W4")
endif()

if (SUPPORT_BC7E)
	if (NOT MSVC)
		add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/bc7e.o ${CMAKE_SOURCE_DIR}/bc7e_avx.o ${CMAKE_SOURCE_DIR}/bc7e_sse2.o ${CMAKE_SOURCE_DIR}/bc7e_sse4.o ${CMAKE_SOURCE_DIR}/bc7e_avx2.o
			COMMAND ispc -g -O2 ${CMAKE_SOURCE_DIR}/bc7e.ispc -o ${CMAKE_SOURCE_DIR}/bc7e.o -h ${CMAKE_SOURCE_DIR}/bc7e_ispc.h --target=sse2,sse4,avx,avx2 --opt=fast-math --opt=disable-assertions
										 	DEPENDS bc7e.ispc)
	else()
		add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/bc7e.obj ${CMAKE_SOURCE_DIR}/bc7e_avx.obj ${CMAKE_SOURCE_DIR}/bc7e_sse2.obj ${CMAKE_SOURCE_DIR}/bc7e_sse4.obj ${CMAKE_SOURCE_DIR}/bc7e_avx2.obj
			COMMAND ispc -g -O2 ${CMAKE_SOURCE_DIR}/bc7e.ispc -o ${CMAKE_SOURCE_DIR}/bc7e.obj -h ${CMAKE_SOURCE_DIR}/bc7e_ispc.h --target=sse2,sse4,avx,avx2 --opt=fast-math --opt=disable-assertions
										 	DEPENDS bc7e.ispc)
	endif()
endif()

# -fno-strict-aliasing shouldn't be necessary, it's here because that is what MSVC uses by default and that's what I've tested with the most.
if (NOT MSVC)
	set(GCC_COMPILE_FLAGS "-fno-strict-aliasing -Wall -Wextra")
	if (NOT BUILD_X64)
		set(GCC_COMPILE_FLAGS "${GCC_COMPILE_FLAGS} -m32")
	endif()
endif()	

set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} ${GCC_COMPILE_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE  "${CMAKE_C_FLAGS_RELEASE} ${GCC_COMPILE_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${GCC_COMPILE_FLAGS} -D_DEBUG")

if (SUPPORT_BC7E)
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COMPILE_FLAGS} -DSUPPORT_BC7E=1")
else()
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COMPILE_FLAGS}")
endif()

set(CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS_RELEASE} ${GCC_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${GCC_COMPILE_FLAGS} -D_DEBUG")

set (BC7ENC_SRC_LIST ${COMMON_SRC_LIST}
	bc7enc.cpp
	bc7enc.h
	bc7decomp.cpp
	bc7decomp_ref.cpp
	bc7decomp.h
	lodepng.cpp	
	lodepng.h
	test.cpp
	rgbcx.cpp
	rgbcx.h
	rgbcx_table4.h
	rgbcx_table4_small.h
	utils.cpp
	utils.h
	miniz.h
	dds_defs.h
	ert.cpp
	ert.h
	rdo_bc_encoder.cpp
	rdo_bc_encoder.h
	bc7e.ispc
	)

if (NOT SUPPORT_BC7E)
		add_executable(bc7enc ${BC7ENC_SRC_LIST})
else()
	if (NOT MSVC)
		add_executable(bc7enc ${BC7ENC_SRC_LIST} ${CMAKE_SOURCE_DIR}/bc7e.o ${CMAKE_SOURCE_DIR}/bc7e_avx.o ${CMAKE_SOURCE_DIR}/bc7e_avx2.o ${CMAKE_SOURCE_DIR}/bc7e_sse2.o ${CMAKE_SOURCE_DIR}/bc7e_sse4.o)
	else()
		add_executable(bc7enc ${BC7ENC_SRC_LIST} ${CMAKE_SOURCE_DIR}/bc7e.obj ${CMAKE_SOURCE_DIR}/bc7e_avx.obj ${CMAKE_SOURCE_DIR}/bc7e_avx2.obj ${CMAKE_SOURCE_DIR}/bc7e_sse2.obj ${CMAKE_SOURCE_DIR}/bc7e_sse4.obj)
	endif()
endif()

if (NOT MSVC)
	target_link_libraries(bc7enc m)
endif()


