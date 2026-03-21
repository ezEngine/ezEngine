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
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <math.h>
#include <assert.h>
#include <algorithm>
#include <vector>
#include <chrono>
#include <random>
#include <filesystem>
#include <inttypes.h>

#include "../CullingThreadpool.h"
#include "../MaskedOcclusionCulling.h"
#include "../FrameRecorder.h"

#if !MOC_RECORDER_ENABLE
#error This project needs to be compiled with MOC_RECORDER_ENABLE set to 1
#endif

#if !MOC_RECORDER_ENABLE_PLAYBACK
#error This project needs to be compiled with MOC_RECORDER_ENABLE_PLAYBACK set to 1
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Image utility functions, minimal BMP writer and depth buffer tone mapping
////////////////////////////////////////////////////////////////////////////////////////

static void WriteBMP( const char *filename, const unsigned char *data, int w, int h )
{
    short header[] = { 0x4D42, 0, 0, 0, 0, 26, 0, 12, 0, (short)w, (short)h, 1, 24 };
#pragma warning ( suppress : 4996 )
    FILE * f;
    if( fopen_s( &f, filename, "wb" ) == 0 )
    {
        fwrite( header, 1, sizeof( header ), f );
        fwrite( data, 1, w * h * 3, f );
        fclose( f );
    }
    else
    {
        printf( "\nError trying to save to %s", filename );
    }
}

static void TonemapDepth( float *depth, unsigned char *image, int w, int h )
{
    // Find min/max w coordinate (discard cleared pixels)
    float minW = FLT_MAX, maxW = 0.0f;
    for( int i = 0; i < w*h; ++i )
    {
        if( depth[i] > 0.0f )
        {
            minW = std::min( minW, depth[i] );
            maxW = std::max( maxW, depth[i] );
        }
    }

    // Tonemap depth values
    for( int i = 0; i < w*h; ++i )
    {
        int intensity = 0;
        if( depth[i] > 0 )
            intensity = (unsigned char)( 223.0*( depth[i] - minW ) / ( maxW - minW ) + 32.0 );

        image[i * 3 + 0] = intensity;
        image[i * 3 + 1] = intensity;
        image[i * 3 + 2] = intensity;
    }
}

struct BenchStats
{
    double  Time;
    int     ClearCount;
    int64_t TriangleCount;
    int64_t RectTestCount;
};

BenchStats BenchmarkRecording( FrameRecording & recording, MaskedOcclusionCulling & moc, const int loopCount, const bool includeTests = false )
{
    assert( !includeTests ); // not yet implemented

    BenchStats stats; memset( &stats, 0, sizeof(stats ) );

	auto before = std::chrono::high_resolution_clock::now();

    moc.SetNearClipPlane( recording.mNearClipPlane );

    for( int loop = 0; loop < loopCount; loop++ )
    {
        // enforce first clear
        moc.ClearBuffer( );
        stats.ClearCount++;

        for( int i = 0; i < recording.mPlaybackOrder.size( ); i++ )
        {
            char elementType = recording.mPlaybackOrder[i].first;
            int elementIndex = recording.mPlaybackOrder[i].second;
            switch( elementType )
            {
                case( 0 ):  // RenderTriangles
                {
                    const FrameRecording::TrianglesEntry & triangleEntry = recording.mTriangleEntries[elementIndex];
                    moc.RenderTriangles( triangleEntry.mVertices.data( ), triangleEntry.mTriangles.data( ), (int)triangleEntry.mTriangles.size( ) / 3, ( triangleEntry.mHasModelToClipMatrix ) ? ( triangleEntry.mModelToClipMatrix ) : ( nullptr ), triangleEntry.mbfWinding, triangleEntry.mClipPlaneMask, triangleEntry.mVertexLayout );
                } break;
                case( 1 ):  // TestRect
                {
                } break;
                case( 2 ):  // TestTriangles
                {
                } break;
                case( 3 ):  // ClearBuffer
                {
                    if( i != 0 )    // skip if first clear because we enforced that anyway
                    {
                        moc.ClearBuffer( );
                        stats.ClearCount++;
                    }
                } break;
                default: assert( false );
            };
        }
    }
	
    auto after = std::chrono::high_resolution_clock::now();

    stats.Time = std::chrono::duration<double>( after - before ).count( );

    return stats;
}

BenchStats BenchmarkRecording( FrameRecording & recording, CullingThreadpool & mocThreadpool, const int loopCount, const bool includeTests = false )
{
    assert( !includeTests ); // not yet implemented

    BenchStats stats; memset( &stats, 0, sizeof( stats ) );

    auto before = std::chrono::high_resolution_clock::now( );

    mocThreadpool.SetNearClipPlane( recording.mNearClipPlane );

    for( int loop = 0; loop < loopCount; loop++ )
    {
        // enforce first clear
        mocThreadpool.ClearBuffer( );
        stats.ClearCount++;

        for( int i = 0; i < recording.mPlaybackOrder.size( ); i++ )
        {
            char elementType = recording.mPlaybackOrder[i].first;
            int elementIndex = recording.mPlaybackOrder[i].second;
            switch( elementType )
            {
                case( 0 ):  // RenderTriangles
                {
                    const FrameRecording::TrianglesEntry & triangleEntry = recording.mTriangleEntries[elementIndex];
                    mocThreadpool.SetMatrix( ( triangleEntry.mHasModelToClipMatrix ) ? ( triangleEntry.mModelToClipMatrix ) : ( nullptr ) );
                    mocThreadpool.SetVertexLayout( triangleEntry.mVertexLayout );
                    assert( !triangleEntry.mHasScissorRect );   // can't use scissor rect in multithreaded scenario because it's already used by the binning part of the algorithm
                    mocThreadpool.RenderTriangles( triangleEntry.mVertices.data( ), triangleEntry.mTriangles.data( ), (int)triangleEntry.mTriangles.size( ) / 3, triangleEntry.mbfWinding );
                    stats.TriangleCount += triangleEntry.mTriangles.size( );
                } break;
                case( 1 ):  // TestRect
                {
                } break;
                case( 2 ):  // TestTriangles
                {
                } break;
                case( 3 ):  // ClearBuffer
                {
                    if( i != 0 )    // skip if first clear because we enforced that anyway
                    {
                        mocThreadpool.ClearBuffer( );
                        stats.ClearCount++;
                    }
                } break;
                default: assert( false ); break;
            };
        }
        mocThreadpool.Flush( );
    }

	auto after = std::chrono::high_resolution_clock::now();

	stats.Time = std::chrono::duration<double>(after - before).count();

    return stats;
}


/*
double BenchmarkTrianglesD3D(ID3D11Buffer *buf, int numTriangles, bool color)
{
	// set the render target as the back buffer
	if (color)
		context->OMSetRenderTargets(1, &textureRTV, textureDSV);
	else
		context->OMSetRenderTargets(0, nullptr, textureDSV);

	// clear the back buffer to a deep blue
	float clearColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
	context->ClearRenderTargetView(textureRTV, clearColor);
	context->ClearDepthStencilView(textureDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Setup primitivelist
	UINT stride = sizeof(float) * 4, offset = 0;
	context->IASetVertexBuffers(0, 1, &buf, &stride, &offset);
	context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Flush();

	// Draw triangles
	auto before = std::chrono::high_resolution_clock::now();
	context->Begin(endQuery);
	context->Draw(numTriangles * 3, 0);
	context->End(endQuery);
	while (context->GetData(endQuery, nullptr, 0, 0) == S_FALSE) {}
	auto after = std::chrono::high_resolution_clock::now();

	return std::chrono::duration<double>(after - before).count();
}
*/

////////////////////////////////////////////////////////////////////////////////////////
// Perform basic fillrate benchmarks for GPU and compare with this libray
////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
    // settings
	const int width             = 1920;
    const int height            = 1080;
    const float nearClip        = 0.1f;
    const int benchLoopCount    = 1000;

	// Flush denorms to zero to avoid performance issues with small values
	_mm_setcsr(_mm_getcsr() | 0x8040);

	MaskedOcclusionCulling *moc = MaskedOcclusionCulling::Create();

	// Initialize directx
	// InitD3D(width, height);

	// Setup and state related code
	////////////////////////////////////////////////////////////////////////////////////////

	// Setup a rendertarget and near clip plane 
	moc->SetResolution(width, height);
	moc->SetNearClipPlane(nearClip);

    printf( "Masked Occlusion Culling performance tester\n" );

    printf( "Compiler: " );
#ifdef __clang__
    printf( "clang/llvm\n" );
#else
    printf( "msvc\n" );
#endif

    printf( "Instruction set: " );
    if( moc->GetImplementation( ) == MaskedOcclusionCulling::SSE2 )     printf( "SSE2\n" );
    if( moc->GetImplementation( ) == MaskedOcclusionCulling::SSE41 )    printf( "SSE41\n" );
    if( moc->GetImplementation( ) == MaskedOcclusionCulling::AVX2 )     printf( "AVX2\n" );
    if( moc->GetImplementation( ) == MaskedOcclusionCulling::AVX512 )   printf( "AVX512\n" );

    printf( "\nMOC resolution used: %d x %d\n", width, height );

    namespace fs = ::std::experimental::filesystem;

    std::vector< std::pair< std::string, FrameRecording > > recordedFiles;
    
    printf( "\nLoading all '*.mocrec' files in the working directory...\n" );
    for( auto & p : fs::directory_iterator( "./" ) )
    {
        if( fs::is_regular_file( p ) && p.path( ).extension( ) == ".mocrec" )
        {
            std::string fileName = p.path( ).string( );
            FrameRecording record;
            if( FrameRecording::Load( fileName.c_str(), record ) )
            {
                recordedFiles.push_back( std::make_pair( fileName, std::move(record) ) );
                printf( " loaded dataset %d from '%s': OK\n", int(recordedFiles.size()-1), fileName.c_str( ) );
            }
            else
            {
                printf( " loading of '%s' failed!\n", fileName.c_str() );
            }
        }
    }

    printf( "\nSaving playback screenshots for loaded recording entries\n" );
    for( int i = 0; i < (int)recordedFiles.size(); i++ ) 
    {
        auto & entry = recordedFiles[i];
        moc->ClearBuffer();

        // Save previously loaded recording entries - useful to keep existing data alive when changing/expanding storage format
#define RESAVE_ENTRIES 0
#if RESAVE_ENTRIES != 0
        moc->RecorderStart( (entry.first).c_str() );
#endif
        const FrameRecording & recording = entry.second;
        moc->SetNearClipPlane(recording.mNearClipPlane);
        for( int i = 0; i < recording.mPlaybackOrder.size(); i++ )
        {
            char elementType = recording.mPlaybackOrder[i].first;
            int elementIndex = recording.mPlaybackOrder[i].second;
            switch( elementType )
            {
                case( 0 ):  // RenderTriangles
                {
                    const FrameRecording::TrianglesEntry & triangleEntry = recording.mTriangleEntries[elementIndex]; 
                    moc->RenderTriangles( triangleEntry.mVertices.data(), triangleEntry.mTriangles.data(), (int)triangleEntry.mTriangles.size()/3, (triangleEntry.mHasModelToClipMatrix)?(triangleEntry.mModelToClipMatrix):(nullptr), triangleEntry.mbfWinding, triangleEntry.mClipPlaneMask, triangleEntry.mVertexLayout );
                } break;
                case( 1 ):  // TestRect
                {
                } break;
                case( 2 ):  // TestTriangles
                {
                } break;
                case( 3 ):  // ClearBuffer
                {
                    moc->ClearBuffer( );
                } break;
                default: assert( false );
            }
        }
#if RESAVE_ENTRIES != 0
        moc->RecorderStop();
#endif

        char fileName[1024]; sprintf_s( fileName, sizeof( fileName ), "%s.bmp", entry.first.c_str() );

        // Compute a per pixel depth buffer from the hierarchical depth buffer, used for visualization.
        float *perPixelZBuffer = new float[width * height];
        moc->ComputePixelDepthBuffer( perPixelZBuffer, true );

        // Tonemap the image
        unsigned char *image = new unsigned char[width * height * 3];
        TonemapDepth( perPixelZBuffer, image, width, height );
        WriteBMP( fileName, image, width, height );
        delete[] image;

        printf( " %d - '%s' written.\n", i, fileName );
    }


	////////////////////////////////////////////////////////////////////////////////////////
	// Perform benchmarks
	////////////////////////////////////////////////////////////////////////////////////////
/*
	printf("\nD3D Z only\n");
	printf("----\n");
	for (int i = 0; i < numSizes; ++i)
	{
		int size = sizes[i];
		double t = BenchmarkTrianglesD3D(vBuffers[i], numTriangles[i], false);
		double GPixelsPerSecond = (double)numTriangles[i] * size*size / (2.0 * 1e9 * t);
		double MTrisPerSecond = (double)numTriangles[i] / (1e6 * t);
		printf("Tri: %3dx%3d - Time: %7.2f ms, MTris/s: %6.2f GPixels/s: %5.2f \n", size, size, t * 1000.0f, MTrisPerSecond, GPixelsPerSecond);
	}
    */

    std::vector<double> singleThreadedTimes;
    singleThreadedTimes.resize( recordedFiles.size() );

	printf("\nSingle threaded benchmark (%d loops of each frame capture)\n", benchLoopCount);
    for( int i = 0; i < (int)recordedFiles.size( ); i++ )
    {
        auto & entry = recordedFiles[i];
		BenchStats stats = BenchmarkRecording( entry.second, *moc, benchLoopCount );

		float MTrisPerSecond = (float)((double)stats.TriangleCount / (1e6 * stats.Time));
		printf(" %d - tris: %12" PRId64 ",  MTris/s: %6.2f,  total time: %9.3fms,  single loop time: %6.3fms\n", i, stats.TriangleCount, MTrisPerSecond, float(stats.Time*1000.0f), float(stats.Time*1000.0/double(benchLoopCount)) );

        singleThreadedTimes[i] = stats.Time;
	}
    printf( "----\n" );

    int numThreads = std::thread::hardware_concurrency( );
    printf( "\nMulti threaded benchmark (%d loops of each frame capture, %d threads)\n", benchLoopCount, numThreads );
    CullingThreadpool ctp( numThreads, 2, numThreads );
    ctp.SetBuffer( moc );
    ctp.WakeThreads( );
    for( int i = 0; i < (int)recordedFiles.size( ); i++ )
    {
        auto & entry = recordedFiles[i];
        BenchStats stats = BenchmarkRecording( entry.second, ctp, benchLoopCount );

        float MTrisPerSecond = (float)( (double)stats.TriangleCount / ( 1e6 * stats.Time ) );
        printf( " %d - tris: %12" PRId64 ",  MTris/s: %6.2f,  total time: %9.3fms,  single loop time: %6.3fms,  MT scaling: %.3fx\n", i, stats.TriangleCount, MTrisPerSecond, float( stats.Time*1000.0f ), float( stats.Time*1000.0 / double( benchLoopCount ) ), singleThreadedTimes[i] / stats.Time );
    }
    ctp.SuspendThreads( );
    printf( "----\n" );
}