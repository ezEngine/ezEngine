
#pragma once

#include <CoreUtils/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/Memory/MemoryUtils.h>

/// \brief Helper template class to iterate over stream elements.
template<typename Type>
class ezProcessingStreamIterator
{
  public:

    /// \brief Constructor.
    ezProcessingStreamIterator( const ezProcessingStream* pStream, ezUInt64 uiNumElements, ezUInt64 uiStartIndex = 0 );

    /// \brief Returns a reference to the current element. Note that the behavior is undefined if HasReachedEnd() is true!
    Type& Current() const;

    /// \brief Returns true of the iterator has reached the end of the stream or the number of elements it should iterate over.
    bool HasReachedEnd() const;

    /// \brief Advances the current pointer to the next element in the stream.
    void Advance();

    // TODO: Add iterator interface? Only makes really sense for element spawners and processors which work on a single stream

  protected:

    void* m_pCurrentPtr;
    void* m_pEndPtr;

    ezUInt64 m_uiElementStride;
};

#include <CoreUtils/DataProcessing/Stream/Implementation/ProcessingStreamIterator_inl.h>
