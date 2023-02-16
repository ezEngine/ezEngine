#ifndef AE_FOUNDATION_CONTAINERS_ARRAY_INL
#define AE_FOUNDATION_CONTAINERS_ARRAY_INL

#include "../../Memory/MemoryManagement.h"
#include "../../Math/Math.h"
#include "../../Memory/Memory.h"
#include "../../Basics/Checks.h"

namespace AE_NS_FOUNDATION
{
  template<class TYPE>
  void aeArray<TYPE>::operator= (const aeArray<TYPE>& cc)
  {
    clear (true);

    m_uiSize = cc.m_uiSize;
    m_fSizeIncrement = cc.m_fSizeIncrement;
    m_uiCapacity = cc.m_uiCapacity;

    if (m_uiCapacity > 0)
      m_pData = AE_MEMORY_ALLOCATE_TYPE_NODEBUG (TYPE, m_uiCapacity, true);

    if (m_uiSize > 0)
      aeMemoryManagement::CopyConstruct<TYPE> (m_pData, cc.m_pData, m_uiSize);
  }

  template<class TYPE>
  aeArray<TYPE>::aeArray (void) : m_pData (nullptr), m_uiSize (0), m_uiCapacity (0), m_fSizeIncrement (1.5f)
  {
  }

  template<class TYPE>
  aeArray<TYPE>::aeArray (const aeArray<TYPE>& cc) : m_pData (nullptr), m_uiSize (0), m_uiCapacity (0), m_fSizeIncrement (1.5f)
  {
    operator= (cc);
  }

  template<class TYPE>
  aeArray<TYPE>::aeArray (aeUInt32 uiSize) : m_pData (nullptr), m_uiSize (0), m_uiCapacity (0), m_fSizeIncrement (1.5f)
  {
    resize (uiSize);
  }

  template<class TYPE>
  aeArray<TYPE>::aeArray (aeUInt32 uiSize, TYPE Init) : m_pData (nullptr), m_uiSize (0), m_uiCapacity (0), m_fSizeIncrement (1.5f)
  {
    resize (uiSize, Init);
  }


  template<class TYPE>
  aeArray<TYPE>::~aeArray ()
  {
    clear (true);
  }

  template<class TYPE>
  void aeArray<TYPE>::clear (bool bDeallocateData)
  {
    if (m_uiSize > 0)
      aeMemoryManagement::Destruct<TYPE> (m_pData, m_uiSize);

    m_uiSize = 0;

    if (bDeallocateData)
    {
      AE_MEMORY_DEALLOCATE_NODEBUG (m_pData, true);

      m_pData = nullptr;
    
      m_uiCapacity = 0;
    }
  }

  template<class TYPE>
  void aeArray<TYPE>::resize (aeUInt32 uiNewSize, TYPE Init)
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
      pNewData = AE_MEMORY_ALLOCATE_TYPE_NODEBUG (TYPE, uiNewSize, true);

      if (m_pData)
      {
        // copy the old data into the new array
        const aeUInt32 uiCopyData = aeMath::Min (uiNewSize, m_uiSize);

        //! \todo If the data is RAW use a memcpy
        //aeMemory::Copy (m_pData,	pNewData, sizeof (TYPE) * uiCopyData);
        aeMemoryManagement::CopyConstruct<TYPE> (pNewData, m_pData, uiCopyData);
        aeMemoryManagement::Destruct<TYPE> (m_pData, m_uiSize);

        // deallocate the old array
        AE_MEMORY_DEALLOCATE_NODEBUG (m_pData, true);
      }
    }

    // initialize all new elements
    if (uiNewSize > m_uiSize)
      aeMemoryManagement::Construct<TYPE> (&pNewData[m_uiSize], uiNewSize - m_uiSize, Init);


    // store the current state
    m_pData = pNewData;
    m_uiSize = uiNewSize;
    m_uiCapacity = uiNewSize;
  }

  template<class TYPE>
  void aeArray<TYPE>::resize (aeUInt32 uiNewSize)
  {
    resize (uiNewSize, TYPE ());
  }

  template<class TYPE>
  void aeArray<TYPE>::reserve (aeUInt32 uiCapacity)
  {
    if (m_uiCapacity >= uiCapacity)
      return;

    TYPE* pNewData = AE_MEMORY_ALLOCATE_TYPE_NODEBUG (TYPE, uiCapacity, true);

    if (m_pData)
    {
      //! \todo If the data is RAW use a memcpy
      //aeMemory::Copy (m_pData,	pNewData, sizeof (TYPE) * m_uiSize);
      aeMemoryManagement::CopyConstruct<TYPE> (pNewData, m_pData, m_uiSize);
      aeMemoryManagement::Destruct<TYPE> (m_pData, m_uiSize);

      AE_MEMORY_DEALLOCATE_NODEBUG (m_pData, true);
    }

    m_pData = pNewData;
    m_uiCapacity = uiCapacity;
  }

  template<class TYPE>
  aeUInt32 aeArray<TYPE>::size (void) const
  {
    return (m_uiSize);
  }

  template<class TYPE>
  aeUInt32 aeArray<TYPE>::capacity (void) const
  {
    return (m_uiCapacity);
  }

  template<class TYPE>
  const TYPE* aeArray<TYPE>::data (void) const
  {
    if (m_uiSize == 0)
      return (nullptr);

    return (m_pData);
  }

  template<class TYPE>
  TYPE* aeArray<TYPE>::data (void)
  {
    if (m_uiSize == 0)
      return (nullptr);

    return (m_pData);
  }

  template<class TYPE>
  const TYPE& aeArray<TYPE>::operator[](aeUInt32 uiIndex) const
  {
    AE_CHECK_DEV (m_pData != nullptr, "aeArray::operator[]: Trying to access an element of an array that has not been initialized.");
    AE_CHECK_DEV (uiIndex < m_uiSize, "aeArray::operator[]: Trying to access an element %d of an array that holds only %d elements.", uiIndex, m_uiSize);

    return (m_pData[uiIndex]);
  }

  template<class TYPE>
  TYPE& aeArray<TYPE>::operator[](aeUInt32 uiIndex)
  {
    AE_CHECK_DEV (m_pData != nullptr, "aeArray::operator[]: Trying to access an element of an array that has not been initialized.");
    AE_CHECK_DEV (uiIndex < m_uiSize, "aeArray::operator[]: Trying to access an element %d of an array that holds only %d elements.", uiIndex, m_uiSize);

    return (m_pData[uiIndex]);
  }

  template<class TYPE>
  void aeArray<TYPE>::SetSizeIncrementPolicy_Multiply (float fSizeMultiple)
  {
    AE_CHECK_DEV (fSizeMultiple > 0, "aeArray::SetSizeIncrementPolicy_Multiply: Value cannot be zero or negative.");

    m_fSizeIncrement = fSizeMultiple;
  }

  template<class TYPE>
  void aeArray<TYPE>::SetSizeIncrementPolicy_Add (aeUInt32 uiAddElements)
  {
    AE_CHECK_DEV (uiAddElements > 0, "aeArray::SetSizeIncrementPolicy_Add: Value cannot be zero.");

    m_fSizeIncrement = - ((float) (uiAddElements));
  }

  template<class TYPE>
  void aeArray<TYPE>::EnsureFreeElements (aeUInt32 uiFreeAtEnd)
  {
    if (m_uiSize + uiFreeAtEnd <= m_uiCapacity)
      return;

    aeUInt32 uiNewSize = m_uiCapacity;
    while (m_uiSize + uiFreeAtEnd > uiNewSize)
    {
      if (m_fSizeIncrement > 0)
        uiNewSize = 1 + aeMath::FloatToInt ((float) (uiNewSize) * m_fSizeIncrement);
      else
        uiNewSize += 1 + aeMath::FloatToInt (-m_fSizeIncrement);
    }

    reserve (uiNewSize);
  }

  template<class TYPE>
  void aeArray<TYPE>::push_back (void)
  {
    EnsureFreeElements (1);

    aeMemoryManagement::Construct<TYPE> (&m_pData[m_uiSize], 1);
    ++m_uiSize;
  }

  template<class TYPE>
  void aeArray<TYPE>::push_back (const TYPE& element)
  {
    EnsureFreeElements (1);

    aeMemoryManagement::CopyConstruct<TYPE> (&m_pData[m_uiSize], element);
    ++m_uiSize;
  }

  template<class TYPE>
  void aeArray<TYPE>::pop_back (void)
  {
    AE_CHECK_DEV (m_pData != nullptr, "aeArray::pop_back: Array is not initialized.");
    AE_CHECK_DEV (m_uiSize > 0, "aeArray::pop_back: Array has a size of zero.");

    --m_uiSize;
    aeMemoryManagement::Destruct<TYPE> (&m_pData[m_uiSize], 1);
  }

  template<class TYPE>
  bool aeArray<TYPE>::empty (void) const
  {
    return (m_uiSize == 0);
  }

  template<class TYPE>
  TYPE& aeArray<TYPE>::front (void)
  {
    AE_CHECK_DEV (m_pData != nullptr, "aeArray::front: Array is not initialized.");
    AE_CHECK_DEV (m_uiSize > 0, "aeArray::front: Array has a size of zero.");

    return (m_pData[0]);
  }

  template<class TYPE>
  TYPE& aeArray<TYPE>::back (void)
  {
    AE_CHECK_DEV (m_pData != nullptr, "aeArray::back: Array is not initialized.");
    AE_CHECK_DEV (m_uiSize > 0, "aeArray::back: Array has a size of zero.");

    return (m_pData[m_uiSize - 1]);
  }

  template<class TYPE>
  const TYPE& aeArray<TYPE>::front (void) const
  {
    AE_CHECK_DEV (m_pData != nullptr, "aeArray::front: Array is not initialized.");
    AE_CHECK_DEV (m_uiSize > 0, "aeArray::front: Array has a size of zero.");

    return (m_pData[0]);
  }

  template<class TYPE>
  const TYPE& aeArray<TYPE>::back (void) const
  {
    AE_CHECK_DEV (m_pData != nullptr, "aeArray::back: Array is not initialized.");
    AE_CHECK_DEV (m_uiSize > 0, "aeArray::back: Array has a size of zero.");

    return (m_pData[m_uiSize - 1]);
  }

}

#endif

