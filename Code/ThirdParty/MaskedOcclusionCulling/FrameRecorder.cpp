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
#include "FrameRecorder.h"

#if MOC_RECORDER_ENABLE
#include <assert.h>
#include <algorithm>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Masked occlusion culling 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool MaskedOcclusionCulling::RecorderStart( const char * outputFilePath ) const
{
    std::lock_guard<std::mutex> lock( mRecorderMutex );

    assert( mRecorder == nullptr ); // forgot to call RecorderStop?
    if( mRecorder != nullptr )
        return false;

#if MOC_RECORDER_USE_STDIO_FILE
    FILE * f;
    if( fopen_s( &f, outputFilePath, "wb" ) != 0 )
        return false;
#else
    std::ofstream outStream( outputFilePath, std::ios::out | std::ios::trunc | std::ios::binary );
    if( !outStream.is_open( ) )
        return false;
#endif
    mRecorder = (FrameRecorder *)mAlignedAllocCallback( 64, sizeof( FrameRecorder ) );
#if MOC_RECORDER_USE_STDIO_FILE
    new (mRecorder) FrameRecorder( f, *this );
#else
    new (mRecorder) FrameRecorder( std::move( outStream ), *this );
#endif

    return true;
}

void MaskedOcclusionCulling::RecorderStop( ) const
{
    std::lock_guard<std::mutex> lock( mRecorderMutex );

    mRecorder->~FrameRecorder();
    mAlignedFreeCallback( mRecorder );
    mRecorder = nullptr;
}

void MaskedOcclusionCulling::RecordRenderTriangles( const float *inVtx, const unsigned int *inTris, int nTris, const float *modelToClipMatrix, ClipPlanes clipPlaneMask, BackfaceWinding bfWinding, const VertexLayout &vtxLayout, CullingResult cullingResult )
{
    std::lock_guard<std::mutex> lock( mRecorderMutex );
    if( mRecorder != nullptr ) 
        mRecorder->RecordRenderTriangles( cullingResult, inVtx, inTris, nTris, modelToClipMatrix, clipPlaneMask, bfWinding, vtxLayout );
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Masked occlusion culling recorder
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if MOC_RECORDER_USE_STDIO_FILE
FrameRecorder::FrameRecorder( FILE *& outFile, const MaskedOcclusionCulling & moc )
#else
FrameRecorder::FrameRecorder( std::ofstream && outStream, const MaskedOcclusionCulling & moc ) : mOutStream( std::move( outStream ) )
#endif
{
#if MOC_RECORDER_USE_STDIO_FILE
    mOutFile = outFile;
    outFile = 0;
    assert( mOutFile != 0 );
#else
    assert( mOutStream.is_open( ) );
#endif

    // for file verification purposes
    unsigned int fileHeader = 0xA701B600;
    Write( &fileHeader, sizeof( fileHeader ) );

    // save some of the MOC states (we can override them for the playback)
    float nearClipPlane = moc.GetNearClipPlane();
    unsigned int width;
    unsigned int height;
    moc.GetResolution( width, height );

    Write( &nearClipPlane, sizeof( nearClipPlane ) );
    Write( &width, sizeof( width ) );
    Write( &height, sizeof( height ) );
}

FrameRecorder::~FrameRecorder( )
{
    // end of file marker
    char footer = 0x7F;
    Write( &footer, 1 );

#if MOC_RECORDER_USE_STDIO_FILE
    fclose( mOutFile );
#else
    mOutStream.close( );
#endif
}

void FrameRecorder::Write( const void * buffer, size_t size )
{
#if MOC_RECORDER_USE_STDIO_FILE
    fwrite( buffer, 1, size, mOutFile );
#else
    mOutStream.write( (const char *)buffer, size );
#endif
}

void FrameRecorder::WriteTriangleRecording( MaskedOcclusionCulling::CullingResult cullingResult, const float *inVtx, const unsigned int *inTris, int nTris, const float *modelToClipMatrix, MaskedOcclusionCulling::ClipPlanes clipPlaneMask, MaskedOcclusionCulling::BackfaceWinding bfWinding, const MaskedOcclusionCulling::VertexLayout & vtxLayout )
{
    // write culling result
    Write( &cullingResult, sizeof( cullingResult ) );

    unsigned int minVIndex = 0xffffffff;
    unsigned int maxVIndex = 0;
    for( int i = 0; i < nTris; i++ )
    {
        const unsigned int & a = inTris[i * 3 + 0];
        const unsigned int & b = inTris[i * 3 + 1];
        const unsigned int & c = inTris[i * 3 + 2];
        minVIndex = std::min( std::min( minVIndex, a ), std::min( b, c ) );
        maxVIndex = std::max( std::max( maxVIndex, a ), std::max( b, c ) );
    }

    // write actually used vertex count
    int vertexCount = ( maxVIndex < minVIndex ) ? ( 0 ) : ( maxVIndex - minVIndex + 1 );
    Write( &vertexCount, sizeof( vertexCount ) );

    // nothing more to write? early exit
    if( vertexCount == 0 )
        return;

    // write vertex size
    int vertexSize = vtxLayout.mStride;
    Write( &vertexSize, sizeof( vertexSize ) );

    // write vertices
    Write( ( (const char*)inVtx ) + minVIndex*vertexSize, vertexSize * ( vertexCount ) );

    // write triangle count
    Write( &nTris, sizeof( nTris ) );

    // write indices with adjusted offset
    for( int i = 0; i < nTris; i++ )
    {
        unsigned int triangleIndices[3];
        triangleIndices[0] = inTris[i * 3 + 0] - minVIndex;
        triangleIndices[1] = inTris[i * 3 + 1] - minVIndex;
        triangleIndices[2] = inTris[i * 3 + 2] - minVIndex;
        Write( triangleIndices, sizeof( triangleIndices ) );
    }

    // write model to clip matrix (if any)
    char hasMatrix = ( modelToClipMatrix != nullptr ) ? ( 1 ) : ( 0 );
    Write( &hasMatrix, sizeof( hasMatrix ) );
    if( hasMatrix )
        Write( modelToClipMatrix, 16 * sizeof( float ) );

    Write( &clipPlaneMask, sizeof( clipPlaneMask ) );

    Write( &bfWinding, sizeof( bfWinding ) );

    // write vertex layout
    Write( &vtxLayout, sizeof( vtxLayout ) );
}

namespace 
{ 
    // Warning, takes ownership of the underlying stream and closes it at the end
    struct InStreamWrapper
    {
#if MOC_RECORDER_USE_STDIO_FILE
        FILE *              mInFile;
#else
        std::ifstream       mInStream;
#endif

#if MOC_RECORDER_USE_STDIO_FILE
        InStreamWrapper( FILE *& inFile )           { mInFile = inFile; assert( mInFile != 0 ); inFile = 0; }
        ~InStreamWrapper( )                         { fclose( mInFile ); }
#else
        InStreamWrapper( std::ifstream && inStream ) : mInStream( std::move( inStream ) ) { assert( mInStream.is_open() ); }
#endif

        size_t Read( void * buffer, size_t size )
        {
#if MOC_RECORDER_USE_STDIO_FILE
            return fread( buffer, 1, size, mInFile );
#else
            mInStream.read( (char*)buffer, size );
            return mInStream.gcount( );
#endif
        }
    };
}

void FrameRecorder::RecordClearBuffer( )
{
    char header = 3;
    Write( &header, 1 );
}

void FrameRecorder::RecordRenderTriangles( MaskedOcclusionCulling::CullingResult cullingResult, const float *inVtx, const unsigned int *inTris, int nTris, const float *modelToClipMatrix, MaskedOcclusionCulling::ClipPlanes clipPlaneMask, MaskedOcclusionCulling::BackfaceWinding bfWinding, const MaskedOcclusionCulling::VertexLayout & vtxLayout )
{
    char header = 0;
    Write( &header, 1 );
    WriteTriangleRecording( cullingResult, inVtx, inTris, nTris, modelToClipMatrix, clipPlaneMask, bfWinding, vtxLayout );
}

void FrameRecorder::RecordTestRect( MaskedOcclusionCulling::CullingResult cullingResult, float xmin, float ymin, float xmax, float ymax, float wmin )
{
    char header = 1;
    Write( &header, 1 );

    Write( &cullingResult, sizeof( cullingResult ) );
    Write( &xmin, sizeof( xmin ) );
    Write( &ymin, sizeof( ymin ) );
    Write( &xmax, sizeof( xmax ) );
    Write( &ymax, sizeof( ymax ) );
    Write( &wmin, sizeof( wmin ) );
}

void FrameRecorder::RecordTestTriangles( MaskedOcclusionCulling::CullingResult cullingResult, const float *inVtx, const unsigned int *inTris, int nTris, const float *modelToClipMatrix, MaskedOcclusionCulling::ClipPlanes clipPlaneMask, MaskedOcclusionCulling::BackfaceWinding bfWinding, const MaskedOcclusionCulling::VertexLayout & vtxLayout )
{
    char header = 2;
    Write( &header, 1 );
    WriteTriangleRecording( cullingResult, inVtx, inTris, nTris, modelToClipMatrix, clipPlaneMask, bfWinding, vtxLayout );
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Masked occlusion culling recording
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#if MOC_RECORDER_ENABLE_PLAYBACK

static bool ReadTriangleRecording( FrameRecording::TrianglesEntry & outEntry, InStreamWrapper & inStream )
{
    // read culling result
    if( inStream.Read( (char*)&outEntry.mCullingResult, sizeof( outEntry.mCullingResult ) ) != sizeof( outEntry.mCullingResult ) )
    {
        assert( false );
        return false;
    }

    // read used vertex count
    int vertexCount = 0;
    if( inStream.Read( (char*)&vertexCount, sizeof( vertexCount ) ) != sizeof( vertexCount ) )
    {
        assert( false );
        return false;
    }

    // nothing in the recording? that's ok, just exit
    if( vertexCount == 0 )
    {
        outEntry.mVertices.clear( );
        outEntry.mTriangles.clear( );
        return true;
    }


    // read vertex size
    int vertexSize = 0;
    if( inStream.Read( (char*)&vertexSize, sizeof( vertexSize ) ) != sizeof( vertexSize ) )
    {
        assert( false );
        return false;
    }

    // read vertices
    outEntry.mVertices.resize( vertexSize / 4 * vertexCount );  // pre-allocate data
    if( inStream.Read( (char*)outEntry.mVertices.data( ), vertexSize * vertexCount ) != ( vertexSize * vertexCount ) )
    {
        assert( false );
        return false;
    }

    // read triangle count
    int triangleCount = 0;
    if( inStream.Read( (char*)&triangleCount, sizeof( triangleCount ) ) != sizeof( triangleCount ) )
    {
        assert( false );
        return false;
    }

    outEntry.mTriangles.resize( triangleCount * 3 );
    if( inStream.Read( (char*)outEntry.mTriangles.data( ), triangleCount * 3 * 4 ) != ( triangleCount * 3 * 4 ) )
    {
        assert( false );
        return false;
    }

    // read matrix (if any)
    char hasMatrix = 0;
    if( inStream.Read( (char*)&hasMatrix, sizeof( hasMatrix ) ) != sizeof( hasMatrix ) )
    {
        assert( false );
        return false;
    }

    if( ( outEntry.mHasModelToClipMatrix = ( hasMatrix != 0 ) ) )
    {
        if( inStream.Read( (char*)outEntry.mModelToClipMatrix, 16 * sizeof( float ) ) != 16 * sizeof( float ) )
        {
            assert( false );
            return false;
        }
    }
    else
    {
        memset( outEntry.mModelToClipMatrix, 0, 16 * sizeof( float ) );
    }

    if( inStream.Read( (char*)&outEntry.mClipPlaneMask, sizeof( outEntry.mClipPlaneMask ) ) != sizeof( outEntry.mClipPlaneMask ) )
    {
        assert( false );
        return false;
    }

    // read triangle cull winding
    if( inStream.Read( (char*)&outEntry.mbfWinding, sizeof( outEntry.mbfWinding ) ) != sizeof( outEntry.mbfWinding ) )
    {
        assert( false );
        return false;
    }

    // read vertex layout
    if( inStream.Read( (char*)&outEntry.mVertexLayout, sizeof( outEntry.mVertexLayout ) ) != sizeof( outEntry.mVertexLayout ) )
    {
        assert( false );
        return false;
    }
    if( outEntry.mVertexLayout.mStride != vertexSize )
    {
        assert( false );
        return false;
    }
    return true;
}

bool FrameRecording::Load( const char * inputFilePath, FrameRecording & outRecording )
{
    outRecording.Reset();

#if MOC_RECORDER_USE_STDIO_FILE
    FILE * inIOFile = 0;
    if( fopen_s( &inIOFile, inputFilePath, "rb" ) != 0 )
        return false;
    InStreamWrapper inStream( inIOFile );
#else
    std::ifstream inIOStream( inputFilePath, std::ios::binary );
    if( !inIOStream.is_open( ) )
    {
        return false;
    }
    InStreamWrapper inStream( std::move(inIOStream) );
#endif
    
    // for file verification purposes
    unsigned int fileHeader = 0;
    if( ( inStream.Read( (char *)&fileHeader, sizeof( fileHeader ) ) != 4 ) || (fileHeader != 0xA701B600) )
    {
        assert( false );
        return false;
    }

    if( inStream.Read( (char *)&outRecording.mNearClipPlane,    sizeof( outRecording.mNearClipPlane )       ) != sizeof( outRecording.mNearClipPlane )    )     { assert( false ); return false; }
    if( inStream.Read( (char *)&outRecording.mResolutionWidth,  sizeof( outRecording.mResolutionWidth )     ) != sizeof( outRecording.mResolutionWidth )  )     { assert( false ); return false; }
    if( inStream.Read( (char *)&outRecording.mResolutionHeight, sizeof( outRecording.mResolutionHeight )    ) != sizeof( outRecording.mResolutionHeight ) )     { assert( false ); return false; }

    bool continueLoading = true;
    while( continueLoading )
    {
        char chunkHeader = 0;
        if( inStream.Read( (char *)&chunkHeader, sizeof( chunkHeader ) ) != 1 )
        {
            assert( false );
            outRecording.Reset();
            return false;
        }
        switch( chunkHeader )
        {
            case( 0 ):      // RenderTriangles
            case( 2 ):      // TestTriangles
            {
                outRecording.mTriangleEntries.push_back( TrianglesEntry() );
                int triangleEntryIndex = (int)outRecording.mTriangleEntries.size( )-1;
                if( !ReadTriangleRecording( outRecording.mTriangleEntries[triangleEntryIndex], inStream ) )
                { 
                    assert( false );
                    outRecording.Reset( );
                    return false;
                }
                outRecording.mPlaybackOrder.push_back( std::make_pair( chunkHeader, triangleEntryIndex )  );
            } break;
            case( 1 ):      // TestRect
            {
                outRecording.mRectEntries.push_back( RectEntry( ) );
                int rectEntryIndex = (int)outRecording.mRectEntries.size( )-1;

                // read rectangle in one go
                if( inStream.Read( (char*)&outRecording.mRectEntries[rectEntryIndex], sizeof( outRecording.mRectEntries[rectEntryIndex] ) ) != sizeof( outRecording.mRectEntries[rectEntryIndex] ) )
                {
                    assert( false );
                    outRecording.Reset( );
                    return false;
                }
                outRecording.mPlaybackOrder.push_back( std::make_pair( chunkHeader, rectEntryIndex ) );
            } break;
            case( 3 ):      // ClearBuffer
            {
                outRecording.mPlaybackOrder.push_back( std::make_pair( 3, -1 ) );
            } break;
            case( 0x7F ):   // eOF
            {
                continueLoading = false;
                return true;
            } break;
            default:
            {
                assert( false );
                outRecording.Reset( );
                return false;
            }
        }
    }

    assert( false );    // we should never get here
    return true;
}

#endif // #if MOC_RECORDER_ENABLE_PLAYBACK

#endif // #if MOC_RECORDER_ENABLE