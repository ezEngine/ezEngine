#ifndef AE_FOUNDATION_CONTAINERS_STACK_H
#define AE_FOUNDATION_CONTAINERS_STACK_H

#include "Deque.h"

namespace AE_NS_FOUNDATION
{
  //! A stack-container. Similar to STL::stack
  /*! A stack only allows to push elements on its top, inspect that element and remove it.
      The stack is implemented on top of a deque, so it has the same performance characteristics.
      Basically this class is only a convenience interface.
  */
  template<class TYPE, aeUInt32 CHUNK_SIZE = ((4096 / sizeof (TYPE)) < 32) ? 32 : (4096 / sizeof (TYPE)), bool NO_DEBUG_ALLOCATOR = true>
  class aeStack
  {
  public:
    //! Initializes the stack to be empty.
    aeStack (void);
    //! Copies the data from the given stack into this one.
    aeStack (const aeStack<TYPE, CHUNK_SIZE, NO_DEBUG_ALLOCATOR>& cc);
    //! Destroys all elements.
    ~aeStack ();

    //! Clears all elements from the stack, leaving it empty.
    void clear (void);

    //! Returns the number of elements on the stack.
    aeUInt32 size (void) const;

    //! Pushes a default-constructed element onto the stack.
    void push (void);
    //! Pushes a copy-constructed element onto the stack.
    void push (const TYPE& element);
    //! Removes the top-most element from the stack.
    void pop (void);

    //! Grants read/write-access to the top-most element.
    TYPE& top (void);
    //! Grants read-access to the top-most element.
    const TYPE& top (void) const;

    //! Returns whether there are zero elements on the stack.
    bool empty (void) const;

    //! Copies the data from the given stack into this one.
    void operator= (const aeStack<TYPE, CHUNK_SIZE, NO_DEBUG_ALLOCATOR>& cc);

  private:
    //! The deque that holds all the stack data.
    aeDeque<TYPE, CHUNK_SIZE, true, NO_DEBUG_ALLOCATOR> m_Data;
  };

}

#include "Inline/Stack.inl"

#endif

