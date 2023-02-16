#ifndef AE_FOUNDATION_CONTAINERS_HYBRIDARRAY_H
#define AE_FOUNDATION_CONTAINERS_HYBRIDARRAY_H

#include "../Defines.h"

namespace AE_NS_FOUNDATION
{

	template<class TYPE, aeUInt32 SIZE>
	class aeHybridArray
	{
	public:
		//! Initializes the array to be empty.
		aeHybridArray(void);
		//! Copies the data from the given array into this one.
		aeHybridArray (const aeHybridArray<TYPE, SIZE>& cc);
		//! Initializes the array with the given size and default-constructed elements.
		aeHybridArray (aeUInt32 uiSize);
		//! Destructs all elements and deallocates all data.
		~aeHybridArray ();

		//! Resizes the array to the given size. ALWAYS reallocates! (except for very rare cases)
		void resize (aeUInt32 uiNewSize);
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

		//! Appends a default-constructed element to the back of the array. Reallocates if necessary.
		void push_back (void);
		//! Appends a copy-constructed element to the back of the array. Reallocates if necessary.
		void push_back (const TYPE& element);
		//! Removes the last element from the array. Never reallocates.
		void pop_back (void);

		//! Copies the data from the other array into this one.
		void operator= (const aeHybridArray<TYPE, SIZE>& cc);

  private:
		//! Grows the array if there are not enough elements free at its end.
		void EnsureFreeElements (aeUInt32 uiFreeAtEnd);

		//! The fixed size array.
    aeUInt8 m_StaticData[SIZE * sizeof (TYPE)];

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

#include "Inline/HybridArray.inl"

#endif


