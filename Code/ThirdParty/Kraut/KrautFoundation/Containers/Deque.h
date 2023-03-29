#ifndef AE_FOUNDATION_CONTAINERS_DEQUE_H
#define AE_FOUNDATION_CONTAINERS_DEQUE_H

#include "../Defines.h"

namespace AE_NS_FOUNDATION
{
  //! A double-ended queue, similar to STL::deque
  /*! A deque allows to add and remove objects at the front and back end.
      It does not allow to insert or erase objects from somewhere else.\n
      Deques allow random-access like arrays in constant time (O (1)). \n
      Access is very fast, though a bit slower than pure array access.\n
      Deques are very well suited if random-access is required and the number of to-be-stored 
      elements is not known before-hand and can vary greatly,
      such that it is difficult to use an array. Deques are "arrays-of-arrays", that means they
      do not reallocate the entire array when they need to grow or shrink.\n
      That is why it is not necessary to "reserve" elements on a deque.\n
      For more information look here:\n
      http://www.codeproject.com/KB/stl/vector_vs_deque.aspx\n
      \n
      Deques use "Chunks" of elements internally. Each time a deque needs to grow, a new chunk is
      allocated. Typically one chunk is roughly 4KB and holds as many elements, as fit in there,
      though a Chunk holds at least 32 elements.\n
      You can override the number of elements per chunk using the CHUNK_SIZE template argument.\n
      \n
      The CONSTRUCT template argument is used for internal purposes.
  */
  template<class TYPE, aeUInt32 CHUNK_SIZE = ((4096 / sizeof (TYPE)) < 32) ? 32 : (4096 / sizeof (TYPE)), bool CONSTRUCT = true, bool NO_DEBUG_ALLOCATOR = true>
  class aeDeque
  {
  public:
    //! Initializes the deque to be empty.
    aeDeque (void);
    //! Copies the data from another deque into this one.
    aeDeque (const aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>& cc);
    //! Initializes the deque with the given size. Elements are default-constructed.
    aeDeque (aeUInt32 uiInitSize);
    //! Destroys the deque and destructs all elements.
    ~aeDeque ();

    //! Sets the number of elements in the deque. Creates are destroys elements at/from the end, if necessary.
    void resize (aeUInt32 uiSize);

    //! Sets the deques size to zero and destroys all elements. Deallocates ALL data, making memory consumption as low as possible.
    void clear (void);

    //! Returns the number of elements in the deque.
    aeUInt32 size (void) const;

    //! Returns whether size is zero.
    bool empty (void) const;

    //! Allows random-access to the n-th element.
    const TYPE& operator[](aeUInt32 uiIndex) const;
    //! Allows random-access to the n-th element.
    TYPE& operator[](aeUInt32 uiIndex);

    //! Appends a default-constructed element to the end of the deque.
    void push_back (void);
    //! Appends a copy-constructed element to the end of the deque.
    void push_back (const TYPE& element);
    //! Removes the very last element from the deque.
    void pop_back (void);

    //! Appends a default-constructed element to the front of the deque.
    void push_front (void);
    //! Appends a copy-constructed element to the front of the deque.
    void push_front (const TYPE& element);
    //! Removes the very first element from the deque.
    void pop_front (void);

    //! Allows read-access to the very first element.
    const TYPE& front (void) const;
    //! Allows read-access to the very last element.
    const TYPE& back (void) const;
    //! Allows read/write-access to the very first element.
    TYPE& front (void);
    //! Allows read/write-access to the very last element.
    TYPE& back (void);

    //! Copies the data from the other deque into this one.
    void operator= (const aeDeque<TYPE, CHUNK_SIZE, CONSTRUCT, NO_DEBUG_ALLOCATOR>& cc);

  private:
    //! One Chunk of data. Stores CHUNK_SIZE elements.
    struct Chunk
    {
      TYPE m_Data[CHUNK_SIZE];
    };

    //! Returns how many chunks are currently needed to store all elements.
    aeUInt32 ComputeChunksUsed (void) const;
    //! Returns how many chunks would be needed to store n elements.
    aeUInt32 ComputeChunksUsed (aeUInt32 uiAtSize) const;

    //! Reallocates the bookkeeping data and allocates/deallocates chunks.
    void resize_chunks (aeUInt32 uiChunks);

    //! Array-of-arrays of chunks.
    Chunk** m_pChunks;
    //! Number of chunks currently allocated.
    aeUInt32 m_uiChunks;

    //! The index of the first element that is actually used.
    aeUInt32 m_uiFirstElement;
    //! The number of elements currently in use.
    aeUInt32 m_uiSize;
  };

}

#include "Inline/Deque.inl"

#endif

