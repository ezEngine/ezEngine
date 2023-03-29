#ifndef AE_FOUNDATION_CONTAINERS_ARRAY_H
#define AE_FOUNDATION_CONTAINERS_ARRAY_H

#include "../Defines.h"

namespace AE_NS_FOUNDATION
{

  //! A dynamically resizable array. Similar to STL::vector
  /*! This container allows random-access to its element. It stores its data internally as an array,
      which means access is equally fast. The array can be dynamically resized, although that is slow,
      because it means to copy all data to a new location.
      One can "reserve" a size and then append elements without the need for reallocation until that
      reserved size is reached. Removing elements from the back also never triggers a reallocation.
  */
  template<class TYPE>
  class aeArray
  {
  public:
    //! Initializes the array to be empty.
    aeArray (void);
    //! Copies the data from the given array into this one.
    aeArray (const aeArray<TYPE>& cc);
    //! Initializes the array with the given size and default-constructed elements.
    aeArray (aeUInt32 uiSize);
    //! Initializes the array with the given size and default-constructed elements.
    aeArray (aeUInt32 uiSize, TYPE Init);
    //! Destructs all elements and deallocates all data.
    ~aeArray ();

    //! Resizes the array to the given size. ALWAYS reallocates! (except for very rare cases)
    void resize (aeUInt32 uiNewSize);
    //! Resizes the array to the given size. ALWAYS reallocates! (except for very rare cases)
    void resize (aeUInt32 uiNewSize, TYPE Init);
    //! Reserves the desired amount of space. Reallocates if the number of elements is larger than the current capacity.
    void reserve (aeUInt32 uiCapacity);

    //! Clears and deletes all data. If bDeallocateData is true, reduces memory consumption to zero. Otherwise keeps all data (reserved size).
    void clear (bool bDeallocateData = true);

    //! Returns the number of elements in the array.
    aeUInt32 size (void) const;
    //! Returns the number of elements the array could hold without needing to reallocate.
    aeUInt32 capacity (void) const;

    //! Returns the pointer to the first element or nullptr if none exists. Safe way to say &operator[0].
    const TYPE* data (void) const;
    //! Returns the pointer to the first element or nullptr if none exists. Safe way to say &operator[0].
    TYPE* data (void);

    //! Gives read-access to the n-th element.
    const TYPE& operator[](aeUInt32 uiIndex) const;
    //! Gives read/write-access to the n-th element.
    TYPE& operator[](aeUInt32 uiIndex);

    //! Appends a default-constructed element to the back of the array. Reallocates if necessary.
    void push_back (void);
    //! Appends a copy-constructed element to the back of the array. Reallocates if necessary.
    void push_back (const TYPE& element);
    //! Removes the last element from the array. Never reallocates.
    void pop_back (void);

    //! If the array needs to be grown, its size will be multiplied by this factor. Mutually exclusive to SetSizeIncrementPolicy_Add.
    void SetSizeIncrementPolicy_Multiply (float fSizeMultiple);
    //! If the array needs to be grown, its size will be incremented by this value. Mutually exclusive to SetSizeIncrementPolicy_Multiply.
    void SetSizeIncrementPolicy_Add (aeUInt32 uiAddElements);

    //! Returns whether there are zero elements stored in the array.
    bool empty (void) const;

    //! Gives read-access to the very first element.
    const TYPE& front (void) const;
    //! Gives read-access to the very last element.
    const TYPE& back (void) const;

    //! Gives read/write-access to the very first element.
    TYPE& front (void);
    //! Gives read/write-access to the very last element.
    TYPE& back (void);

    //! Copies the data from the other array into this one.
    void operator= (const aeArray<TYPE>& cc);

  private:
    //! Grows the array if there are not enough elements free at its end.
    void EnsureFreeElements (aeUInt32 uiFreeAtEnd);

    //! Pointer to the data (array).
    TYPE* m_pData;
    //! The current number of elements stored in the array.
    aeUInt32 m_uiSize;
    //! The maximum possible number of elements that can be stored.
    aeUInt32 m_uiCapacity;
    //! Positive/Negative value that indicates the policy to use, when the array needs to be grown.
    float m_fSizeIncrement;
  };

}

#include "Inline/Array.inl"

#endif

