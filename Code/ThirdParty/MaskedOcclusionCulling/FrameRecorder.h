////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License.  You may obtain a copy
// of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
////////////////////////////////////////////////////////////////////////////////
#pragma once

 /*!
  * \file FrameRecorder.h
  * \brief Masked occlusion culling recorder class (set MOC_RECORDER_ENABLE to 1 to enable)
  *
  * Masked occlusion culling recorder class (To enable, set MOC_RECORDER_ENABLE to 1 in MaskedOcclusionCulling.h)
  *
  * Enables gathering and storing all triangle rendering and all testing calls and their results to a file, for
  * later playback and performance testing.
  * Usage info:
  *  - Calling MaskedOcclusionCulling::RecorderStart with a file name will open the file and start recording all subsequent
  *    triangle rendering and any testing calls including the test results, and MaskedOcclusionCulling::RecorderStop will 
  *    stop recording and close the file.
  *  - ClearBuffer-s are not recorded so if recording multiple frames, Start/RecorderStop is needed for each frame. For 
  *    correctness testing, the recording should be started around or after ClearBuffer, before any other "Render" calls.
  *  - BinTriangles and RenderTrilist calls are NOT recorded; If using a custom multithreaded rendering, one should 
  *    record input triangles by manually calling MaskedOcclusionCulling::RecordRenderTriangles - see 
  *    CullingThreadpool::RenderTriangles for an example. 
  *    This is done intentionally in order to get raw input triangles so we can performance test and optimize various
  *    thread pool approaches.
  */

#include "MaskedOcclusionCulling.h"

#ifndef MOC_RECORDER_USE_STDIO_FILE
/*!
 * Whether to use FILE or std::ofstream/istream for file access (to avoid compatibility issues in some environments)
 */
#define MOC_RECORDER_USE_STDIO_FILE         1
#endif

#if MOC_RECORDER_ENABLE

#if MOC_RECORDER_USE_STDIO_FILE
#include <stdio.h>
#else
#include <fstream>
#endif

#include <mutex>

class FrameRecorder
{
#if MOC_RECORDER_USE_STDIO_FILE
    FILE *              mOutFile;
#else
    std::ofstream       mOutStream;
#endif

protected:
	friend class MaskedOcclusionCulling;
#if MOC_RECORDER_USE_STDIO_FILE
    FrameRecorder( FILE *& outFile, const MaskedOcclusionCulling & moc );
#else
	FrameRecorder( std::ofstream && outStream, const MaskedOcclusionCulling & moc );
#endif

public:
    ~FrameRecorder( );

protected:
    void Write( const void * buffer, size_t size );
    void WriteTriangleRecording( MaskedOcclusionCulling::CullingResult cullingResult, const float *inVtx, const unsigned int *inTris, int nTris, const float *modelToClipMatrix, MaskedOcclusionCulling::ClipPlanes clipPlaneMask, MaskedOcclusionCulling::BackfaceWinding bfWinding, const MaskedOcclusionCulling::VertexLayout & vtxLayout );

public:
    void RecordClearBuffer( );
	void RecordRenderTriangles( MaskedOcclusionCulling::CullingResult cullingResult, const float *inVtx, const unsigned int *inTris, int nTris, const float *modelToClipMatrix, MaskedOcclusionCulling::ClipPlanes clipPlaneMask, MaskedOcclusionCulling::BackfaceWinding bfWinding, const MaskedOcclusionCulling::VertexLayout &vtxLayout );
	void RecordTestRect( MaskedOcclusionCulling::CullingResult cullingResult, float xmin, float ymin, float xmax, float ymax, float wmin );
	void RecordTestTriangles( MaskedOcclusionCulling::CullingResult cullingResult, const float *inVtx, const unsigned int *inTris, int nTris, const float *modelToClipMatrix, MaskedOcclusionCulling::ClipPlanes clipPlaneMask, MaskedOcclusionCulling::BackfaceWinding bfWinding, const MaskedOcclusionCulling::VertexLayout &vtxLayout );
};

#if MOC_RECORDER_ENABLE_PLAYBACK
#include <vector>

#if 0 // For future use - in case all vector uses below need conversion to custom allocator
template <class T>
struct MOCVectorAllocator
{
    const MaskedOcclusionCulling::pfnAlignedAlloc   m_alloc;
    const MaskedOcclusionCulling::pfnAlignedFree    m_free;
    typedef T value_type;
    MOCVectorAllocator( ) = delete;
    MOCVectorAllocator( MaskedOcclusionCulling::pfnAlignedAlloc alloc, MaskedOcclusionCulling::pfnAlignedFree free ) noexcept : m_alloc( alloc ), m_free( free ) { }
    template <class U> constexpr MOCVectorAllocator( const MOCVectorAllocator<U>& c ) noexcept : m_alloc( c.m_alloc ), m_free( c.m_free ) {}
    T* allocate( std::size_t n )
    {
        if( n > std::size_t( -1 ) / sizeof( T ) ) throw std::bad_alloc( );
        if( auto p = static_cast<T*>( m_alloc( 64, n * sizeof( T ) ) ) ) return p;
        throw std::bad_alloc( );
    }
    void deallocate( T* p, std::size_t ) noexcept
    {
        m_free( p );
    }
};
template <class T, class U>
inline bool operator==( const MOCVectorAllocator<T>&, const MOCVectorAllocator<U>& ) { return true; }
template <class T, class U>
inline bool operator!=( const MOCVectorAllocator<T>&, const MOCVectorAllocator<U>& ) { return false; }
#endif

struct FrameRecording
{
	struct TrianglesEntry
	{
		MaskedOcclusionCulling::CullingResult	mCullingResult;
		std::vector<float>						mVertices;
		std::vector<unsigned int>				mTriangles;
		float									mModelToClipMatrix[16];
		MaskedOcclusionCulling::BackfaceWinding	mbfWinding;
		MaskedOcclusionCulling::VertexLayout	mVertexLayout;
		MaskedOcclusionCulling::ClipPlanes		mClipPlaneMask;
		bool									mHasModelToClipMatrix;
		bool									mHasScissorRect;
	};
	struct RectEntry
	{
		MaskedOcclusionCulling::CullingResult mCullingResult;
		float mXMin;
		float mYMin;
		float mXMax;
		float mYMax;
		float mWMin;
    };

	// list of type&index pairs for playback ( type 0 is RenderTriangles, type 1 is TestRect, type 2 is TestTriangles, type 3 is ClearBuffer )
	std::vector< std::pair< char, int > >	mPlaybackOrder;

	std::vector< TrianglesEntry >			mTriangleEntries;
	std::vector< RectEntry >				mRectEntries;

    float                                   mNearClipPlane;
    unsigned int                            mResolutionWidth;
    unsigned int                            mResolutionHeight;

    FrameRecording( ) = default;
    FrameRecording( const FrameRecording & other ) = default;

    FrameRecording( FrameRecording && other )
    {
        mPlaybackOrder = std::move( other.mPlaybackOrder );
        mTriangleEntries = std::move( other.mTriangleEntries );
        mRectEntries = std::move( other.mRectEntries );
        mNearClipPlane = other.mNearClipPlane;
        mResolutionWidth = other.mResolutionWidth;
        mResolutionHeight = other.mResolutionHeight;
    }

	void Reset( )
	{
		mPlaybackOrder.clear();
		mTriangleEntries.clear();
		mRectEntries.clear();
        mNearClipPlane = 0.0f;
        mResolutionWidth = 0;
        mResolutionHeight = 0;
	}

	static bool Load( const char * inputFilePath, FrameRecording & outRecording );
};

#endif // #if MOC_RECORDER_ENABLE_PLAYBACK

#endif // #if MOC_RECORDER_ENABLE