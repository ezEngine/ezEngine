#ifndef AE_FOUNDATION_CONTAINERS_DEQUE_INL
#define AE_FOUNDATION_CONTAINERS_DEQUE_INL

#include "../../Math/Math.h"
#include "../../Memory/MemoryManagement.h"
#include "../../Basics/Checks.h"

namespace AE_NS_FOUNDATION
{
  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::aeDeque (void)
  {
    m_uiFirstElement = 0;
    m_uiSize = 0;
    m_uiChunks = 0;
    m_pChunks = nullptr;
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::aeDeque (const aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>& cc)
  {
    m_uiFirstElement = 0;
    m_uiSize = 0;
    m_uiChunks = 0;
    m_pChunks = nullptr;

    operator= (cc);

    AE_CHECK_DEV (m_uiSize == cc.m_uiSize, "");
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::aeDeque (aeUInt32 uiInitSize)
  {
    m_uiFirstElement = 0;
    m_uiSize = 0;
    m_uiChunks = 0;
    m_pChunks = nullptr;

    if (uiInitSize > 0)
    {
      resize (uiInitSize);

      AE_CHECK_DEV (m_uiFirstElement > 0, "");
      AE_CHECK_DEV (m_uiChunks > 0, "");
      AE_CHECK_DEV (m_pChunks != nullptr, "");
    }

    AE_CHECK_DEV (m_uiSize == uiInitSize, "");
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::~aeDeque ()
  {
    clear ();
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  void aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::operator= (const aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>& cc)
  {
    clear ();

    // TODO: This seems to be majorly broken regarding memory allocations
    // Maybe fixed now

    const aeUInt32 uiFirstChunkUsed = cc.m_uiFirstElement / CHUNK_SIZE;

    const aeUInt32 uiChunksUsed = cc.ComputeChunksUsed ();
    const aeUInt32 uiChunksToAllocate = 16 + uiChunksUsed + 16;

    m_uiSize = cc.m_uiSize;
    m_uiFirstElement = (16 * CHUNK_SIZE) + (cc.m_uiFirstElement % CHUNK_SIZE);

    m_uiChunks = uiChunksToAllocate;

    m_pChunks = (Chunk**) AE_MEMORY_ALLOCATE_TYPE_NODEBUG (Chunk*, uiChunksToAllocate, NO_DEBUG_ALLOCATOR);
    aeMemory::FillWithZeros (&m_pChunks[0], sizeof (Chunk*) * uiChunksToAllocate);

    for (aeUInt32 ui = 0; ui < uiChunksUsed; ++ui)
    {
      m_pChunks[16 + ui] = (Chunk*) AE_MEMORY_ALLOCATE_TYPE_NODEBUG (Chunk, 1, NO_DEBUG_ALLOCATOR);

      aeUInt32 uiFirstElementInChunk = 0;
      aeUInt32 uiElementsInChunk = CHUNK_SIZE;

      if (ui == 0)
      {
        uiFirstElementInChunk = cc.m_uiFirstElement % CHUNK_SIZE;
        uiElementsInChunk -= uiFirstElementInChunk;
      }

      if (ui == uiChunksUsed - 1)
      {
        if (cc.m_uiSize > 0)
          uiElementsInChunk = (cc.m_uiFirstElement + cc.m_uiSize - 1) % CHUNK_SIZE - uiFirstElementInChunk + 1;
        else
          uiElementsInChunk = 0;
      }

      aeMemoryManagement::CopyConstruct<TYPE> (&(m_pChunks[16 + ui]->m_Data[uiFirstElementInChunk]), &(cc.m_pChunks[uiFirstChunkUsed + ui]->m_Data[uiFirstElementInChunk]), uiElementsInChunk);
    }
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  aeUInt32 aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::size (void) const
  {
    return (m_uiSize);
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  bool aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::empty (void) const
  {
    return (m_uiSize == 0);
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  const TYPE& aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::front (void) const
  {
    AE_CHECK_DEV (m_uiSize > 0, "aeDeque::front (const): The deque is empty, you cannot access any element.");

    return (operator[](0));
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  const TYPE& aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::back (void) const
  {
    AE_CHECK_DEV (m_uiSize > 0, "aeDeque::back (const): The deque is empty, you cannot access any element.");

    return (operator[](m_uiSize - 1));
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  TYPE& aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::front (void)
  {
    AE_CHECK_DEV (m_uiSize > 0, "aeDeque::front: The deque is empty, you cannot access any element.");

    return (operator[](0));
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  TYPE& aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::back (void)
  {
    AE_CHECK_DEV (m_uiSize > 0, "aeDeque::back: The deque is empty, you cannot access any element.");

    return (operator[](m_uiSize - 1));
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  void aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::resize (aeUInt32 uiSize)
  {
    // DO NOT CLEAR when uiNewSize is 0 !!

    if (CONSTRUCT)
    {
      for (aeUInt32 ui = uiSize; ui < m_uiSize; ++ui)
        aeMemoryManagement::Destruct<TYPE> (&operator[](ui));
    }

    const aeUInt32 uiChunksRequired = ComputeChunksUsed (uiSize);

    resize_chunks (uiChunksRequired);

    const aeUInt32 uiOldSize = m_uiSize;
    m_uiSize = uiSize;

    if (CONSTRUCT)
    {
      for (aeUInt32 ui = uiOldSize; ui < uiSize; ++ui)
        aeMemoryManagement::Construct<TYPE> (&operator[](ui));
    }
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  void aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::clear (void)
  {
    if (CONSTRUCT)
    {
      for (aeUInt32 ui = 0; ui < m_uiSize; ++ui)
        aeMemoryManagement::Destruct<TYPE> (&operator[] (ui));
    }

    for (aeUInt32 ui = 0; ui < m_uiChunks; ++ui)
      AE_MEMORY_DEALLOCATE_NODEBUG (m_pChunks[ui], NO_DEBUG_ALLOCATOR);

    AE_MEMORY_DEALLOCATE_NODEBUG (m_pChunks, NO_DEBUG_ALLOCATOR);
    m_pChunks = nullptr;

    m_uiChunks = 0;
    m_uiFirstElement = 0;
    m_uiSize = 0;
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  void aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::push_back (void)
  {
    const aeUInt32 uiIndex = m_uiFirstElement + m_uiSize;
    const aeUInt32 uiMaxIndex = CHUNK_SIZE * m_uiChunks;

    if (uiIndex >= uiMaxIndex)
    {
      // resize will ALWAYS reallocate and make enough room at the end
      resize (m_uiSize);
    }

    ++m_uiSize;

    if (CONSTRUCT)
      aeMemoryManagement::Construct<TYPE> (&back ());
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  void aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::push_front (void)
  {
    if (m_uiFirstElement == 0)
    {
      // resize will ALWAYS reallocate and make enough room at the beginning
      resize (m_uiSize);
    }

    --m_uiFirstElement;
    ++m_uiSize;

    if (CONSTRUCT)
      aeMemoryManagement::Construct<TYPE> (&front ());
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  void aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::pop_back (void)
  {
    AE_CHECK_DEV (m_uiSize > 0, "aeDeque::pop_back: The deque is empty, you cannot pop any element.");
    
    if (CONSTRUCT)
      aeMemoryManagement::Destruct<TYPE> (&back ());

    --m_uiSize;
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  void aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::pop_front (void)
  {
    AE_CHECK_DEV (m_uiSize > 0, "aeDeque::pop_front: The deque is empty, you cannot pop any element.");

    if (CONSTRUCT)
      aeMemoryManagement::Destruct<TYPE> (&front ());

    --m_uiSize;
    ++m_uiFirstElement;
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  const TYPE& aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::operator[](aeUInt32 uiIndex) const
  {
    AE_CHECK_DEV (m_pChunks != nullptr, "aeDeque::operator[]: Array is nullptr.");
    AE_CHECK_DEV (uiIndex < m_uiSize, "aeDeque::operator[] (const): Element %d is out of range.", uiIndex);

    uiIndex += m_uiFirstElement;

    aeUInt32 uiChunk  = uiIndex / CHUNK_SIZE;
    aeUInt32 uiOffset = uiIndex % CHUNK_SIZE;

    AE_CHECK_DEV (uiChunk < m_uiChunks, "");
    AE_CHECK_DEV (uiOffset < CHUNK_SIZE, "");

    // some functions rely on operator[] to allocate the next chunk
    if (m_pChunks[uiChunk] == nullptr)
      m_pChunks[uiChunk] = AE_MEMORY_ALLOCATE_TYPE_NODEBUG (Chunk, 1, NO_DEBUG_ALLOCATOR);

    return (m_pChunks[uiChunk]->m_Data[uiOffset]);
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  TYPE& aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::operator[](aeUInt32 uiIndex)
  {
    AE_CHECK_DEV (uiIndex < m_uiSize, "aeDeque::operator[]: Element %d is out of range.", uiIndex);
    AE_CHECK_DEV (m_pChunks != nullptr, "aeDeque::operator[]: Array is nullptr.");

    uiIndex += m_uiFirstElement;

    aeUInt32 uiChunk  = uiIndex / CHUNK_SIZE;
    aeUInt32 uiOffset = uiIndex % CHUNK_SIZE;

    AE_CHECK_DEV (uiChunk < m_uiChunks, "");
    AE_CHECK_DEV (uiOffset < CHUNK_SIZE, "");

    // some functions rely on operator[] to allocate the next chunk
    if (m_pChunks[uiChunk] == nullptr)
      m_pChunks[uiChunk] = AE_MEMORY_ALLOCATE_TYPE_NODEBUG (Chunk, 1, NO_DEBUG_ALLOCATOR);

    return (m_pChunks[uiChunk]->m_Data[uiOffset]);
  }


  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  aeUInt32 aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::ComputeChunksUsed (aeUInt32 uiAtSize) const
  {
    if (uiAtSize == 0)
      return (0);

    const aeUInt32 uiFirstIndex = m_uiFirstElement;
    const aeUInt32 uiLastIndex  = m_uiFirstElement + uiAtSize - 1;

    const aeUInt32 uiFirstChunk = uiFirstIndex / CHUNK_SIZE;
    const aeUInt32 uiLastChunk  = uiLastIndex / CHUNK_SIZE;

    return ((uiLastChunk - uiFirstChunk) + 1);
  }


  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  aeUInt32 aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::ComputeChunksUsed (void) const
  {
    return (ComputeChunksUsed (m_uiSize));
  }


  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  void aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::resize_chunks (aeUInt32 uiChunks)
  {
    const aeUInt32 uiFirstChunk = m_uiFirstElement / CHUNK_SIZE;
    const aeUInt32 uiChunksRequired = 16 + uiChunks + 16;
    const aeUInt32 uiChunksInUse = ComputeChunksUsed ();

    // deallocate all chunks that shall not be kept
    if (uiChunks < uiChunksInUse)
    {
      for (aeUInt32 ui = uiFirstChunk + uiChunks; ui < uiFirstChunk + uiChunksInUse; ++ui)
      {
        AE_MEMORY_DEALLOCATE_NODEBUG (m_pChunks[ui], NO_DEBUG_ALLOCATOR);
        m_pChunks[ui] = nullptr;
      }
    }

    // create the new chunk array
    Chunk** pNewChunks = AE_MEMORY_ALLOCATE_TYPE_NODEBUG (Chunk*, uiChunksRequired, NO_DEBUG_ALLOCATOR);
    aeMemory::FillWithZeros (&pNewChunks[0], sizeof (Chunk*) * uiChunksRequired);

    // now copy the chunk pointers, that should be kept
    for (aeUInt32 ui = 0; ui < aeMath::Min (uiChunks, uiChunksInUse); ++ui)
      pNewChunks[16 + ui] = m_pChunks[uiFirstChunk + ui];

    // free the old chunk-pointer array and store the new array in it
    AE_MEMORY_DEALLOCATE_NODEBUG (m_pChunks, NO_DEBUG_ALLOCATOR);

    m_pChunks = pNewChunks;
    m_uiChunks = uiChunksRequired;

    m_uiFirstElement = (16 * CHUNK_SIZE) + (m_uiFirstElement % CHUNK_SIZE);

    // recompute the current size
    // if the number of chunks has grown, the size will not change
    // if it has shrinked, it will be clamped to the maximum number of elements, that could fit in it
    m_uiSize = aeMath::Min (m_uiSize, (uiChunks * CHUNK_SIZE) - (m_uiFirstElement % CHUNK_SIZE));
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  void aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::push_back (const TYPE& element)
  {
    const aeUInt32 uiIndex = m_uiFirstElement + m_uiSize;
    const aeUInt32 uiMaxIndex = CHUNK_SIZE * m_uiChunks;

    if (uiIndex >= uiMaxIndex)
    {
      // resize will ALWAYS reallocate and make enough room at the end
      resize (m_uiSize);
    }

    ++m_uiSize;

    if (CONSTRUCT)
      aeMemoryManagement::CopyConstruct<TYPE> (&back (), element);

    //back () = element;
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool CONSTRUCT, bool NO_DEBUG_ALLOCATOR>
  void aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>::push_front (const TYPE& element)
  {
    if (m_uiFirstElement == 0)
    {
      // resize will ALWAYS reallocate and make enough room at the beginning
      resize (m_uiSize);
    }

    --m_uiFirstElement;
    ++m_uiSize;

    if (CONSTRUCT)
      aeMemoryManagement::CopyConstruct<TYPE> (&front (), element);

    //front () = element;
  }

}

#endif

