#ifndef AE_FOUNDATION_CONTAINERS_STACK_INL
#define AE_FOUNDATION_CONTAINERS_STACK_INL

#include "../../Basics/Checks.h"

namespace AE_NS_FOUNDATION
{
  template<class TYPE, aeUInt32 CHUNK_SIZE, bool NO_DEBUG_ALLOCATOR>
  aeStack<TYPE, CHUNK_SIZE, NO_DEBUG_ALLOCATOR>::aeStack (void)
  {
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool NO_DEBUG_ALLOCATOR>
  aeStack<TYPE, CHUNK_SIZE, NO_DEBUG_ALLOCATOR>::aeStack (const aeStack<TYPE, CHUNK_SIZE, NO_DEBUG_ALLOCATOR>& cc)
  {
    operator= (cc);
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool NO_DEBUG_ALLOCATOR>
  aeStack<TYPE, CHUNK_SIZE, NO_DEBUG_ALLOCATOR>::~aeStack (void)
  {
    clear ();
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool NO_DEBUG_ALLOCATOR>
  void aeStack<TYPE, CHUNK_SIZE, NO_DEBUG_ALLOCATOR>::clear (void)
  {
    m_Data.clear ();
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool NO_DEBUG_ALLOCATOR>
  aeUInt32 aeStack<TYPE, CHUNK_SIZE, NO_DEBUG_ALLOCATOR>::size (void) const
  {
    return (m_Data.size ());
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool NO_DEBUG_ALLOCATOR>
  void aeStack<TYPE, CHUNK_SIZE, NO_DEBUG_ALLOCATOR>::push (void)
  {
    m_Data.push_back (TYPE ());
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool NO_DEBUG_ALLOCATOR>
  void aeStack<TYPE, CHUNK_SIZE, NO_DEBUG_ALLOCATOR>::push (const TYPE& element)
  {
    m_Data.push_back (element);
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool NO_DEBUG_ALLOCATOR>
  void aeStack<TYPE, CHUNK_SIZE, NO_DEBUG_ALLOCATOR>::pop (void)
  {
    AE_CHECK_DEV (!empty (), "aeStack::pop: You cannot pop any element, the stack is empty.");

    m_Data.pop_back ();
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool NO_DEBUG_ALLOCATOR>
  TYPE& aeStack<TYPE, CHUNK_SIZE, NO_DEBUG_ALLOCATOR>::top (void)
  {
    AE_CHECK_DEV (!empty (), "aeStack::top: You cannot access any element, the stack is empty.");

    return (m_Data.back ());
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool NO_DEBUG_ALLOCATOR>
  const TYPE& aeStack<TYPE, CHUNK_SIZE, NO_DEBUG_ALLOCATOR>::top (void) const
  {
    AE_CHECK_DEV (!empty (), "aeStack::top (const): You cannot access any element, the stack is empty.");

    return (m_Data.back ());
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool NO_DEBUG_ALLOCATOR>
  bool aeStack<TYPE, CHUNK_SIZE, NO_DEBUG_ALLOCATOR>::empty (void) const
  {
    return (m_Data.empty ());
  }

  template<class TYPE, aeUInt32 CHUNK_SIZE, bool NO_DEBUG_ALLOCATOR>
  void aeStack<TYPE, CHUNK_SIZE, NO_DEBUG_ALLOCATOR>::operator= (const aeStack<TYPE, CHUNK_SIZE, NO_DEBUG_ALLOCATOR>& cc)
  {
    m_Data = cc.m_Data;
  }
}

#endif

