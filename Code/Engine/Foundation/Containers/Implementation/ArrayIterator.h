
#pragma once

#include <algorithm>

/// \brief Base class for STL like random access iterators
template<class ARRAY, class T, bool reverse = false>
struct const_iterator_base
{
public:
  typedef std::random_access_iterator_tag iterator_category;
  typedef T value_type;
  typedef ptrdiff_t difference_type;
  typedef T const * pointer;
  typedef T const & reference;

  const_iterator_base() { m_Array = nullptr; m_iIndex = 0; }
  const_iterator_base(const ARRAY& deque, size_t index) { m_Array = const_cast<ARRAY*>(&deque); m_iIndex = index; }

  EZ_FORCE_INLINE const_iterator_base& operator++() { m_iIndex += 1; return *this; }
  EZ_FORCE_INLINE const_iterator_base& operator--() { m_iIndex -= 1; return *this; }

  EZ_FORCE_INLINE const_iterator_base operator++(int) { m_iIndex += 1; return const_iterator_base(*m_Array, m_iIndex - 1); }
  EZ_FORCE_INLINE const_iterator_base operator--(int) { m_iIndex -= 1; return const_iterator_base(*m_Array, m_iIndex + 1); }

  EZ_FORCE_INLINE bool operator==(const const_iterator_base& rhs) const { return m_Array == rhs.m_Array && m_iIndex == rhs.m_iIndex; }
  EZ_FORCE_INLINE bool operator!=(const const_iterator_base& rhs) const { return !(*this == rhs); }

  EZ_FORCE_INLINE ptrdiff_t operator-(const const_iterator_base& rhs) const { return m_iIndex - rhs.m_iIndex; }

  EZ_FORCE_INLINE const_iterator_base operator+(ptrdiff_t rhs) const { return const_iterator_base(*m_Array, m_iIndex + rhs); }
  EZ_FORCE_INLINE const_iterator_base operator-(ptrdiff_t rhs) const { return const_iterator_base(*m_Array, m_iIndex - rhs); }

  EZ_FORCE_INLINE void operator+=(ptrdiff_t rhs) { m_iIndex += rhs; }
  EZ_FORCE_INLINE void operator-=(ptrdiff_t rhs) { m_iIndex -= rhs; }

  EZ_FORCE_INLINE const T& operator*() const { if (reverse) return (*m_Array)[m_Array->GetCount() - (ezUInt32)m_iIndex - 1]; else return (*m_Array)[(ezUInt32)m_iIndex]; }
  EZ_FORCE_INLINE const T* operator->() const { return &(**this); }

  EZ_FORCE_INLINE bool operator< (const const_iterator_base& rhs) const { return m_iIndex < rhs.m_iIndex; }
  EZ_FORCE_INLINE bool operator>(const const_iterator_base& rhs) const { return m_iIndex > rhs.m_iIndex; }
  EZ_FORCE_INLINE bool operator<=(const const_iterator_base& rhs) const { return m_iIndex <= rhs.m_iIndex; }
  EZ_FORCE_INLINE bool operator>=(const const_iterator_base& rhs) const { return m_iIndex >= rhs.m_iIndex; }

protected:
  ARRAY* m_Array;
  size_t m_iIndex;
};

/// \brief Non-const STL like iterators
template<class ARRAY, class T, bool reverse = false>
struct iterator_base : public const_iterator_base < ARRAY, T, reverse >
{
public:
  typedef T* pointer;
  typedef T& reference;

  iterator_base() { }
  iterator_base(ARRAY& deque, size_t index) : const_iterator_base<ARRAY, T, reverse>(deque, index) { }

  EZ_FORCE_INLINE iterator_base& operator++() { this->m_iIndex += 1; return *this; }
  EZ_FORCE_INLINE iterator_base& operator--() { this->m_iIndex -= 1; return *this; }

  EZ_FORCE_INLINE iterator_base operator++(int) { this->m_iIndex += 1; return iterator_base(*this->m_Array, this->m_iIndex - 1); }
  EZ_FORCE_INLINE iterator_base operator--(int) { this->m_iIndex -= 1; return iterator_base(*this->m_Array, this->m_iIndex + 1); }

  using const_iterator_base<ARRAY, T, reverse>::operator+;
  using const_iterator_base<ARRAY, T, reverse>::operator-;

  EZ_FORCE_INLINE iterator_base operator+(ptrdiff_t rhs) const { return iterator_base(*this->m_Array, this->m_iIndex + rhs); }
  EZ_FORCE_INLINE iterator_base operator-(ptrdiff_t rhs) const { return iterator_base(*this->m_Array, this->m_iIndex - rhs); }

  using const_iterator_base<ARRAY, T, reverse>::operator*;
  EZ_FORCE_INLINE T& operator*() { if (reverse) return (*this->m_Array)[this->m_Array->GetCount() - (ezUInt32) this->m_iIndex - 1]; else return (*this->m_Array)[(ezUInt32) this->m_iIndex]; }
  using const_iterator_base<ARRAY, T, reverse>::operator->;
  EZ_FORCE_INLINE T* operator->() { return &(**this); }

};

/// \brief Base class for Pointer like reverse iterators
template<class T>
struct const_reverse_pointer_iterator
{
public:
  typedef std::random_access_iterator_tag iterator_category;
  typedef T value_type;
  typedef ptrdiff_t difference_type;
  typedef T* pointer;
  typedef T& reference;

  const_reverse_pointer_iterator() { m_ptr = nullptr; }
  const_reverse_pointer_iterator(T const * ptr) : m_ptr(const_cast<T*>(ptr)) {}

  EZ_FORCE_INLINE const_reverse_pointer_iterator& operator++() { m_ptr--; return *this; }
  EZ_FORCE_INLINE const_reverse_pointer_iterator& operator--() { m_ptr++; return *this; }

  EZ_FORCE_INLINE const_reverse_pointer_iterator operator++(int) { m_ptr--; return const_reverse_pointer_iterator(m_ptr + 1); }
  EZ_FORCE_INLINE const_reverse_pointer_iterator operator--(int) { m_ptr++; return const_reverse_pointer_iterator(m_ptr - 1); }

  EZ_FORCE_INLINE bool operator==(const const_reverse_pointer_iterator& rhs) const { return m_ptr == rhs.m_ptr; }
  EZ_FORCE_INLINE bool operator!=(const const_reverse_pointer_iterator& rhs) const { return m_ptr != rhs.m_ptr; }

  EZ_FORCE_INLINE ptrdiff_t operator-(const const_reverse_pointer_iterator& rhs) const { return rhs.m_ptr - m_ptr; }

  EZ_FORCE_INLINE const_reverse_pointer_iterator operator+(ptrdiff_t rhs) const { return const_reverse_pointer_iterator(m_ptr - rhs); }
  EZ_FORCE_INLINE const_reverse_pointer_iterator operator-(ptrdiff_t rhs) const { return const_reverse_pointer_iterator(m_ptr + rhs); }

  EZ_FORCE_INLINE void operator+=(ptrdiff_t rhs) { m_ptr -= rhs; }
  EZ_FORCE_INLINE void operator-=(ptrdiff_t rhs) { m_ptr += rhs; }

  EZ_FORCE_INLINE const T& operator*() const { return *m_ptr; }
  EZ_FORCE_INLINE const T* operator->() const { return m_ptr; }

  EZ_FORCE_INLINE bool operator< (const const_reverse_pointer_iterator& rhs) const { return m_ptr > rhs.m_ptr; }
  EZ_FORCE_INLINE bool operator>(const const_reverse_pointer_iterator& rhs) const { return m_ptr < rhs.m_ptr; }
  EZ_FORCE_INLINE bool operator<=(const const_reverse_pointer_iterator& rhs) const { return m_ptr >= rhs.m_ptr; }
  EZ_FORCE_INLINE bool operator>=(const const_reverse_pointer_iterator& rhs) const { return m_ptr <= rhs.m_ptr; }

protected:
  T* m_ptr;
};

/// \brief Non-Const class for Pointer like reverse iterators
template<class T>
struct reverse_pointer_iterator : public const_reverse_pointer_iterator<T>
{
public:
  typedef T* pointer;
  typedef T& reference;

  reverse_pointer_iterator() { }
  reverse_pointer_iterator(T* ptr) : const_reverse_pointer_iterator<T>(ptr) { }

  EZ_FORCE_INLINE reverse_pointer_iterator& operator++() { this->m_ptr--; return *this; }
  EZ_FORCE_INLINE reverse_pointer_iterator& operator--() { this->m_ptr++; return *this; }

  EZ_FORCE_INLINE reverse_pointer_iterator operator++(int) { this->m_ptr--; return reverse_pointer_iterator(this->m_ptr + 1); }
  EZ_FORCE_INLINE reverse_pointer_iterator operator--(int) { this->m_ptr++; return reverse_pointer_iterator(this->m_ptr - 1); }

  using const_reverse_pointer_iterator<T>::operator+;
  using const_reverse_pointer_iterator<T>::operator-;

  EZ_FORCE_INLINE reverse_pointer_iterator operator+(ptrdiff_t rhs) const { return reverse_pointer_iterator(this->m_ptr - rhs); }
  EZ_FORCE_INLINE reverse_pointer_iterator operator-(ptrdiff_t rhs) const { return reverse_pointer_iterator(this->m_ptr + rhs); }

  using const_reverse_pointer_iterator<T>::operator*;
  EZ_FORCE_INLINE T& operator*() { return *(this->m_ptr); }
  using const_reverse_pointer_iterator<T>::operator->;
  EZ_FORCE_INLINE T* operator->() { return this->m_ptr; }

};