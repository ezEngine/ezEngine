#ifndef AE_FOUNDATION_CONTAINERS_HYBRIDARRAY_INL
#define AE_FOUNDATION_CONTAINERS_HYBRIDARRAY_INL

#include "../../Memory/MemoryManagement.h"
#include "../../Math/Math.h"
#include "../../Memory/Memory.h"
#include "../../Basics/Checks.h"

namespace AE_NS_FOUNDATION
{
  template<class TYPE, aeUInt32 SIZE>
  void aeHybridArray<TYPE, SIZE>::operator= (const aeHybridArray<TYPE, SIZE>& cc)
  {
    clear (true);

    m_uiSize = cc.m_uiSize;
    m_fSizeIncrement = cc.m_fSizeIncrement;
    m_uiCapacity = cc.m_uiCapacity;

    if (m_uiCapacity > SIZE)
      m_pData = AE_MEMORY_ALLOCATE_TYPE_NODEBUG (TYPE, m_uiCapacity, true);
    else
      m_pData = (TYPE*) &m_StaticData[0];

    if (m_uiSize > 0)
      aeMemoryManagement::CopyConstruct<TYPE> (m_pData, cc.m_pData, m_uiSize);
  }

  template<class TYPE, aeUInt32 SIZE>
  aeHybridArray<TYPE, SIZE>::aeHybridArray (void)
  {
    m_pData = (TYPE*) &m_StaticData[0];
    m_uiSize = 0;
    m_uiCapacity = SIZE;
    m_fSizeIncrement = 1.5f;
  }

  template<class TYPE, aeUInt32 SIZE>
  aeHybridArray<TYPE, SIZE>::aeHybridArray (const aeHybridArray<TYPE, SIZE>& cc)
  {
    m_pData = (TYPE*) &m_StaticData[0];
    m_uiSize = 0;
    m_uiCapacity = SIZE;
    m_fSizeIncrement = 1.5f;

    operator= (cc);
  }

  template<class TYPE, aeUInt32 SIZE>
  aeHybridArray<TYPE, SIZE>::aeHybridArray (aeUInt32 uiSize)
  {
    m_pData = (TYPE*) &m_StaticData[0];
    m_uiSize = 0;
    m_uiCapacity = SIZE;
    m_fSizeIncrement = 1.5f;

    resize (uiSize);
  }

  template<class TYPE, aeUInt32 SIZE>
  aeHybridArray<TYPE, SIZE>::~aeHybridArray ()
  {
    clear (true);
  }

  template<class TYPE, aeUInt32 SIZE>
  void aeHybridArray<TYPE, SIZE>::resize (aeUInt32 uiNewSize)
  {
    if (uiNewSize == 0)
    {
      clear ();
      return;
    }

    if ((uiNewSize == m_uiSize) && (m_uiCapacity == uiNewSize))
      return;

    // destruct all elements, that are not needed anymore
    if (uiNewSize < m_uiSize)
      aeMemoryManagement::Destruct<TYPE> (&m_pData[uiNewSize], m_uiSize - uiNewSize);


    TYPE* pNewData = nullptr;

    if ((uiNewSize <= m_uiCapacity) && (uiNewSize + 32 >= m_uiCapacity))
    {
      // do not reallocate for small changes
      pNewData = m_pData;
    }
    else
    {
      if (uiNewSize <= SIZE)
        pNewData = (TYPE*) &m_StaticData[0];
      else
        pNewData = AE_MEMORY_ALLOCATE_TYPE_NODEBUG (TYPE, uiNewSize, true);

      // if the resized array does not copy from the static buffer into itself
      if (m_pData != pNewData)
      {
        // copy the old data into the new array
        const aeUInt32 uiCopyData = aeMath::Min (uiNewSize, m_uiSize);

        //! \todo If the data is RAW use a memcpy
        //aeMemory::Copy (m_pData,  pNewData, sizeof (TYPE) * uiCopyData);
        aeMemoryManagement::CopyConstruct<TYPE> (pNewData, m_pData, uiCopyData);
        aeMemoryManagement::Destruct<TYPE> (m_pData, m_uiSize);

        // deallocate the old array, if it was not the static buffer
        if (m_pData != (TYPE*) &m_StaticData[0])
          AE_MEMORY_DEALLOCATE_NODEBUG (m_pData, true);
      }
    }

    // initialize all new elements
    if (uiNewSize > m_uiSize)
      aeMemoryManagement::Construct<TYPE> (&pNewData[m_uiSize], uiNewSize - m_uiSize);


    // store the current state
    m_pData = pNewData;
    m_uiSize = uiNewSize;
    m_uiCapacity = aeMath::Max (uiNewSize, SIZE);
  }

  template<class TYPE, aeUInt32 SIZE>
  void aeHybridArray<TYPE, SIZE>::reserve (aeUInt32 uiCapacity)
  {
    if (m_uiCapacity >= uiCapacity)
      return;

    TYPE* pNewData = AE_MEMORY_ALLOCATE_TYPE_NODEBUG (TYPE, uiCapacity, true);

    if (m_pData)
    {
      //! \todo If the data is RAW use a memcpy
      //aeMemory::Copy (m_pData,  pNewData, sizeof (TYPE) * m_uiSize);
      aeMemoryManagement::CopyConstruct<TYPE> (pNewData, m_pData, m_uiSize);
      aeMemoryManagement::Destruct<TYPE> (m_pData, m_uiSize);

      // only deallocate the old data, if it was not the built in buffer
      if (m_uiCapacity > SIZE)
        AE_MEMORY_DEALLOCATE_NODEBUG (m_pData, true);
    }

    m_pData = pNewData;
    m_uiCapacity = uiCapacity;
  }

  template<class TYPE, aeUInt32 SIZE>
  void aeHybridArray<TYPE, SIZE>::clear (bool bDeallocateData)
  {
    if (m_uiSize > 0)
      aeMemoryManagement::Destruct<TYPE> (m_pData, m_uiSize);

    m_uiSize = 0;

    if ((bDeallocateData) && (m_uiCapacity > SIZE))
    {
      AE_MEMORY_DEALLOCATE_NODEBUG (m_pData, true);

      m_pData = (TYPE*) &m_StaticData[0];
      m_uiCapacity = SIZE;
    }
  }

  template<class TYPE, aeUInt32 SIZE>
  void aeHybridArray<TYPE, SIZE>::EnsureFreeElements (aeUInt32 uiFreeAtEnd)
  {
    if (m_uiSize + uiFreeAtEnd <= m_uiCapacity)
      return;

    aeUInt32 uiNewSize = m_uiCapacity;
    while (m_uiSize + uiFreeAtEnd > uiNewSize)
    {
      if (m_fSizeIncrement > 0)
        uiNewSize = 1 + (aeUInt32) aeMath::Trunc ((float) (uiNewSize) * m_fSizeIncrement);
      else
        uiNewSize += 1 + (aeUInt32) aeMath::Trunc (-m_fSizeIncrement);
    }

    reserve (uiNewSize);
  }


  template<class TYPE, aeUInt32 SIZE>
  void aeHybridArray<TYPE, SIZE>::push_back (void)
  {
    EnsureFreeElements (1);

    aeMemoryManagement::Construct<TYPE> (&m_pData[m_uiSize], 1);
    ++m_uiSize;
  }

  template<class TYPE, aeUInt32 SIZE>
  void aeHybridArray<TYPE, SIZE>::push_back (const TYPE& element)
  {
    EnsureFreeElements (1);

    aeMemoryManagement::CopyConstruct<TYPE> (&m_pData[m_uiSize], element);
    ++m_uiSize;
  }

  template<class TYPE, aeUInt32 SIZE>
  void aeHybridArray<TYPE, SIZE>::pop_back (void)
  {
    AE_CHECK_DEV (m_pData != nullptr, "aeHybridArray::pop_back: Array is not initialized.");
    AE_CHECK_DEV (m_uiSize > 0, "aeHybridArray::pop_back: Array has a size of zero.");

    --m_uiSize;
    aeMemoryManagement::Destruct<TYPE> (&m_pData[m_uiSize], 1);
  }


  template<class TYPE, aeUInt32 SIZE>
  aeUInt32 aeHybridArray<TYPE, SIZE>::capacity (void) const
  {
    return (m_uiCapacity);
  }

  template<class TYPE, aeUInt32 SIZE>
  aeUInt32 aeHybridArray<TYPE, SIZE>::size (void) const
  {
    return (m_uiSize);
  }

  template<class TYPE, aeUInt32 SIZE>
  bool aeHybridArray<TYPE, SIZE>::empty (void) const
  {
    return (m_uiSize == 0);
  }

  template<class TYPE, aeUInt32 SIZE>
  const TYPE* aeHybridArray<TYPE, SIZE>::data (void) const
  {
    if (m_uiSize == 0)
      return (nullptr);

    return (m_pData);
  }

  template<class TYPE, aeUInt32 SIZE>
  TYPE* aeHybridArray<TYPE, SIZE>::data (void)
  {
    if (m_uiSize == 0)
      return (nullptr);

    return (m_pData);
  }

  template<class TYPE, aeUInt32 SIZE>
  const TYPE& aeHybridArray<TYPE, SIZE>::operator[](aeUInt32 uiIndex) const
  {
    AE_CHECK_DEV (m_pData != nullptr, "aeHybridArray::operator[]: Trying to access an element of an array that has not been initialized.");
    AE_CHECK_DEV (uiIndex < m_uiSize, "aeHybridArray::operator[]: Trying to access an element %d of an array that holds only %d elements.", uiIndex, m_uiSize);

    return (m_pData[uiIndex]);
  }

  template<class TYPE, aeUInt32 SIZE>
  TYPE& aeHybridArray<TYPE, SIZE>::operator[](aeUInt32 uiIndex)
  {
    AE_CHECK_DEV (m_pData != nullptr, "aeHybridArray::operator[]: Trying to access an element of an array that has not been initialized.");
    AE_CHECK_DEV (uiIndex < m_uiSize, "aeHybridArray::operator[]: Trying to access an element %d of an array that holds only %d elements.", uiIndex, m_uiSize);

    return (m_pData[uiIndex]);
  }

  template<class TYPE, aeUInt32 SIZE>
  void aeHybridArray<TYPE, SIZE>::SetSizeIncrementPolicy_Multiply (float fSizeMultiple)
  {
    AE_CHECK_DEV (fSizeMultiple > 0, "aeHybridArray::SetSizeIncrementPolicy_Multiply: Value cannot be zero or negative.");

    m_fSizeIncrement = fSizeMultiple;
  }

  template<class TYPE, aeUInt32 SIZE>
  void aeHybridArray<TYPE, SIZE>::SetSizeIncrementPolicy_Add (aeUInt32 uiAddElements)
  {
    AE_CHECK_DEV (uiAddElements > 0, "aeHybridArray::SetSizeIncrementPolicy_Add: Value cannot be zero.");

    m_fSizeIncrement = - ((float) (uiAddElements));
  }


  template<class TYPE, aeUInt32 SIZE>
  TYPE& aeHybridArray<TYPE, SIZE>::front (void)
  {
    AE_CHECK_DEV (m_pData != nullptr, "aeHybridArray::front: Array is not initialized.");
    AE_CHECK_DEV (m_uiSize > 0, "aeHybridArray::front: Array has a size of zero.");

    return (m_pData[0]);
  }

  template<class TYPE, aeUInt32 SIZE>
  TYPE& aeHybridArray<TYPE, SIZE>::back (void)
  {
    AE_CHECK_DEV (m_pData != nullptr, "aeHybridArray::back: Array is not initialized.");
    AE_CHECK_DEV (m_uiSize > 0, "aeHybridArray::back: Array has a size of zero.");

    return (m_pData[m_uiSize - 1]);
  }

  template<class TYPE, aeUInt32 SIZE>
  const TYPE& aeHybridArray<TYPE, SIZE>::front (void) const
  {
    AE_CHECK_DEV (m_pData != nullptr, "aeHybridArray::front: Array is not initialized.");
    AE_CHECK_DEV (m_uiSize > 0, "aeHybridArray::front: Array has a size of zero.");

    return (m_pData[0]);
  }

  template<class TYPE, aeUInt32 SIZE>
  const TYPE& aeHybridArray<TYPE, SIZE>::back (void) const
  {
    AE_CHECK_DEV (m_pData != nullptr, "aeHybridArray::back: Array is not initialized.");
    AE_CHECK_DEV (m_uiSize > 0, "aeHybridArray::back: Array has a size of zero.");

    return (m_pData[m_uiSize - 1]);
  }

}

#endif

