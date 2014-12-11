
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
  typedef const_iterator_base* pointer;
  typedef const_iterator_base& reference;

  const_iterator_base() { m_Array = nullptr; m_iIndex = 0; }
  const_iterator_base(const ARRAY& deque, size_t index) { m_Array = const_cast<ARRAY*>(&deque); m_iIndex = index; }

  const_iterator_base& operator++() { m_iIndex += 1; return *this; }
  const_iterator_base& operator--() { m_iIndex -= 1; return *this; }

  const_iterator_base operator++(int) { m_iIndex += 1; return const_iterator_base(*m_Array, m_iIndex - 1); }
  const_iterator_base operator--(int) { m_iIndex -= 1; return const_iterator_base(*m_Array, m_iIndex + 1); }

  bool operator==(const const_iterator_base& rhs) const { return m_Array == rhs.m_Array && m_iIndex == rhs.m_iIndex; }
  bool operator!=(const const_iterator_base& rhs) const { return !(*this == rhs); }

  size_t operator+(const const_iterator_base& rhs) const { return m_iIndex + rhs.m_iIndex; }
  size_t operator-(const const_iterator_base& rhs) const { return m_iIndex - rhs.m_iIndex; }

  const_iterator_base operator+(ptrdiff_t rhs) const { return const_iterator_base(*m_Array, m_iIndex + rhs); }
  const_iterator_base operator-(ptrdiff_t rhs) const { return const_iterator_base(*m_Array, m_iIndex - rhs); }

  void operator+=(ptrdiff_t rhs) { m_iIndex += rhs; }
  void operator-=(ptrdiff_t rhs) { m_iIndex -= rhs; }

  const T& operator*() const { if (reverse) return (*m_Array)[m_Array->GetCount() - (ezUInt32) m_iIndex - 1]; else return (*m_Array)[(ezUInt32) m_iIndex]; }

  bool operator< (const const_iterator_base& rhs) const { return m_iIndex < rhs.m_iIndex; }
  bool operator>(const const_iterator_base& rhs) const { return m_iIndex > rhs.m_iIndex; }
  bool operator<=(const const_iterator_base& rhs) const { return m_iIndex <= rhs.m_iIndex; }
  bool operator>=(const const_iterator_base& rhs) const { return m_iIndex >= rhs.m_iIndex; }

protected:
  ARRAY* m_Array;
  size_t m_iIndex;
};

/// \brief Non-const STL like iterators
template<class ARRAY, class T, bool reverse = false>
struct iterator_base : public const_iterator_base < ARRAY, T, reverse >
{
public:
  typedef iterator_base* pointer;
  typedef iterator_base& reference;

  iterator_base() { }
  iterator_base(ARRAY& deque, size_t index) : const_iterator_base<ARRAY, T, reverse>(deque, index) { }

  iterator_base& operator++() { this->m_iIndex += 1; return *this; }
  iterator_base& operator--() { this->m_iIndex -= 1; return *this; }

  iterator_base operator++(int) { this->m_iIndex += 1; return iterator_base(*this->m_Array, this->m_iIndex - 1); }
  iterator_base operator--(int) { this->m_iIndex -= 1; return iterator_base(*this->m_Array, this->m_iIndex + 1); }

  using const_iterator_base<ARRAY, T, reverse>::operator+;
  using const_iterator_base<ARRAY, T, reverse>::operator-;

  iterator_base operator+(ptrdiff_t rhs) const { return iterator_base(*this->m_Array, this->m_iIndex + rhs); }
  iterator_base operator-(ptrdiff_t rhs) const { return iterator_base(*this->m_Array, this->m_iIndex - rhs); }

  using const_iterator_base<ARRAY, T, reverse>::operator*;
  T& operator*() { if (reverse) return (*this->m_Array)[this->m_Array->GetCount() - (ezUInt32) this->m_iIndex - 1]; else return (*this->m_Array)[(ezUInt32) this->m_iIndex]; }

};
