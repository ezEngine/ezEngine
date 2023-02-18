
#pragma once

#include <algorithm>

/// \brief Base class for STL like random access iterators
template <class ARRAY, class T, bool reverse = false>
struct const_iterator_base
{
public:
  typedef std::random_access_iterator_tag iterator_category;
  using value_type = T;
  using difference_type = ptrdiff_t;
  using pointer = const T*;
  using reference = const T&;

  const_iterator_base()
  {
    m_pArray = nullptr;
    m_uiIndex = 0;
  }
  const_iterator_base(const ARRAY& deque, size_t uiIndex)
  {
    m_pArray = const_cast<ARRAY*>(&deque);
    m_uiIndex = uiIndex;
  }

  EZ_ALWAYS_INLINE const_iterator_base& operator++()
  {
    m_uiIndex += 1;
    return *this;
  }
  EZ_ALWAYS_INLINE const_iterator_base& operator--()
  {
    m_uiIndex -= 1;
    return *this;
  }

  EZ_ALWAYS_INLINE const_iterator_base operator++(int)
  {
    m_uiIndex += 1;
    return const_iterator_base(*m_pArray, m_uiIndex - 1);
  }
  EZ_ALWAYS_INLINE const_iterator_base operator--(int)
  {
    m_uiIndex -= 1;
    return const_iterator_base(*m_pArray, m_uiIndex + 1);
  }

  EZ_ALWAYS_INLINE bool operator==(const const_iterator_base& rhs) const { return m_pArray == rhs.m_pArray && m_uiIndex == rhs.m_uiIndex; }
  EZ_ALWAYS_INLINE bool operator!=(const const_iterator_base& rhs) const { return !(*this == rhs); }

  EZ_ALWAYS_INLINE ptrdiff_t operator-(const const_iterator_base& rhs) const { return m_uiIndex - rhs.m_uiIndex; }

  EZ_ALWAYS_INLINE const_iterator_base operator+(ptrdiff_t rhs) const { return const_iterator_base(*m_pArray, m_uiIndex + rhs); }
  EZ_ALWAYS_INLINE const_iterator_base operator-(ptrdiff_t rhs) const { return const_iterator_base(*m_pArray, m_uiIndex - rhs); }

  EZ_ALWAYS_INLINE void operator+=(ptrdiff_t rhs) { m_uiIndex += rhs; }
  EZ_ALWAYS_INLINE void operator-=(ptrdiff_t rhs) { m_uiIndex -= rhs; }

  inline const T& operator*() const
  {
    if (reverse)
      return (*m_pArray)[m_pArray->GetCount() - (ezUInt32)m_uiIndex - 1];
    else
      return (*m_pArray)[(ezUInt32)m_uiIndex];
  }
  EZ_ALWAYS_INLINE const T* operator->() const { return &(**this); }

  EZ_ALWAYS_INLINE bool operator<(const const_iterator_base& rhs) const { return m_uiIndex < rhs.m_uiIndex; }
  EZ_ALWAYS_INLINE bool operator>(const const_iterator_base& rhs) const { return m_uiIndex > rhs.m_uiIndex; }
  EZ_ALWAYS_INLINE bool operator<=(const const_iterator_base& rhs) const { return m_uiIndex <= rhs.m_uiIndex; }
  EZ_ALWAYS_INLINE bool operator>=(const const_iterator_base& rhs) const { return m_uiIndex >= rhs.m_uiIndex; }

  EZ_ALWAYS_INLINE const T& operator[](size_t uiIndex) const
  {
    if (reverse)
      return (*m_pArray)[m_pArray->GetCount() - static_cast<ezUInt32>(m_uiIndex + uiIndex) - 1];
    else
      return (*m_pArray)[static_cast<ezUInt32>(m_uiIndex + uiIndex)];
  }

protected:
  ARRAY* m_pArray;
  size_t m_uiIndex;
};

/// \brief Non-const STL like iterators
template <class ARRAY, class T, bool reverse = false>
struct iterator_base : public const_iterator_base<ARRAY, T, reverse>
{
public:
  using pointer = T*;
  using reference = T&;

  iterator_base() {}
  iterator_base(ARRAY& ref_deque, size_t uiIndex)
    : const_iterator_base<ARRAY, T, reverse>(ref_deque, uiIndex)
  {
  }

  EZ_ALWAYS_INLINE iterator_base& operator++()
  {
    this->m_uiIndex += 1;
    return *this;
  }
  EZ_ALWAYS_INLINE iterator_base& operator--()
  {
    this->m_uiIndex -= 1;
    return *this;
  }

  EZ_ALWAYS_INLINE iterator_base operator++(int)
  {
    this->m_uiIndex += 1;
    return iterator_base(*this->m_pArray, this->m_uiIndex - 1);
  }
  EZ_ALWAYS_INLINE iterator_base operator--(int)
  {
    this->m_uiIndex -= 1;
    return iterator_base(*this->m_pArray, this->m_uiIndex + 1);
  }

  using const_iterator_base<ARRAY, T, reverse>::operator+;
  using const_iterator_base<ARRAY, T, reverse>::operator-;

  EZ_ALWAYS_INLINE iterator_base operator+(ptrdiff_t rhs) const { return iterator_base(*this->m_pArray, this->m_uiIndex + rhs); }
  EZ_ALWAYS_INLINE iterator_base operator-(ptrdiff_t rhs) const { return iterator_base(*this->m_pArray, this->m_uiIndex - rhs); }

  inline T& operator*() const
  {
    if (reverse)
      return (*this->m_pArray)[this->m_pArray->GetCount() - (ezUInt32)this->m_uiIndex - 1];
    else
      return (*this->m_pArray)[(ezUInt32)this->m_uiIndex];
  }

  EZ_ALWAYS_INLINE T* operator->() const { return &(**this); }

  EZ_ALWAYS_INLINE T& operator[](size_t uiIndex) const
  {
    if (reverse)
      return (*this->m_pArray)[this->m_pArray->GetCount() - static_cast<ezUInt32>(this->m_uiIndex + uiIndex) - 1];
    else
      return (*this->m_pArray)[static_cast<ezUInt32>(this->m_uiIndex + uiIndex)];
  }
};

/// \brief Base class for Pointer like reverse iterators
template <class T>
struct const_reverse_pointer_iterator
{
public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = T;
  using difference_type = ptrdiff_t;
  using pointer = T*;
  using reference = T&;

  const_reverse_pointer_iterator() { m_pPtr = nullptr; }
  const_reverse_pointer_iterator(T const* pPtr)
    : m_pPtr(const_cast<T*>(pPtr))
  {
  }

  EZ_ALWAYS_INLINE const_reverse_pointer_iterator& operator++()
  {
    m_pPtr--;
    return *this;
  }
  EZ_ALWAYS_INLINE const_reverse_pointer_iterator& operator--()
  {
    m_pPtr++;
    return *this;
  }

  EZ_ALWAYS_INLINE const_reverse_pointer_iterator operator++(int)
  {
    m_pPtr--;
    return const_reverse_pointer_iterator(m_pPtr + 1);
  }
  EZ_ALWAYS_INLINE const_reverse_pointer_iterator operator--(int)
  {
    m_pPtr++;
    return const_reverse_pointer_iterator(m_pPtr - 1);
  }

  EZ_ALWAYS_INLINE bool operator==(const const_reverse_pointer_iterator& rhs) const { return m_pPtr == rhs.m_pPtr; }
  EZ_ALWAYS_INLINE bool operator!=(const const_reverse_pointer_iterator& rhs) const { return m_pPtr != rhs.m_pPtr; }

  EZ_ALWAYS_INLINE ptrdiff_t operator-(const const_reverse_pointer_iterator& rhs) const { return rhs.m_pPtr - m_pPtr; }

  EZ_ALWAYS_INLINE const_reverse_pointer_iterator operator+(ptrdiff_t rhs) const { return const_reverse_pointer_iterator(m_pPtr - rhs); }
  EZ_ALWAYS_INLINE const_reverse_pointer_iterator operator-(ptrdiff_t rhs) const { return const_reverse_pointer_iterator(m_pPtr + rhs); }

  EZ_ALWAYS_INLINE void operator+=(ptrdiff_t rhs) { m_pPtr -= rhs; }
  EZ_ALWAYS_INLINE void operator-=(ptrdiff_t rhs) { m_pPtr += rhs; }

  EZ_ALWAYS_INLINE const T& operator*() const { return *m_pPtr; }
  EZ_ALWAYS_INLINE const T* operator->() const { return m_pPtr; }

  EZ_ALWAYS_INLINE bool operator<(const const_reverse_pointer_iterator& rhs) const { return m_pPtr > rhs.m_pPtr; }
  EZ_ALWAYS_INLINE bool operator>(const const_reverse_pointer_iterator& rhs) const { return m_pPtr < rhs.m_pPtr; }
  EZ_ALWAYS_INLINE bool operator<=(const const_reverse_pointer_iterator& rhs) const { return m_pPtr >= rhs.m_pPtr; }
  EZ_ALWAYS_INLINE bool operator>=(const const_reverse_pointer_iterator& rhs) const { return m_pPtr <= rhs.m_pPtr; }

  EZ_ALWAYS_INLINE const T& operator[](ptrdiff_t iIndex) const { return *(m_pPtr - iIndex); }

protected:
  T* m_pPtr;
};

/// \brief Non-Const class for Pointer like reverse iterators
template <class T>
struct reverse_pointer_iterator : public const_reverse_pointer_iterator<T>
{
public:
  using pointer = T*;
  using reference = T&;

  reverse_pointer_iterator() {}
  reverse_pointer_iterator(T* pPtr)
    : const_reverse_pointer_iterator<T>(pPtr)
  {
  }

  EZ_ALWAYS_INLINE reverse_pointer_iterator& operator++()
  {
    this->m_pPtr--;
    return *this;
  }
  EZ_ALWAYS_INLINE reverse_pointer_iterator& operator--()
  {
    this->m_pPtr++;
    return *this;
  }

  EZ_ALWAYS_INLINE reverse_pointer_iterator operator++(int)
  {
    this->m_pPtr--;
    return reverse_pointer_iterator(this->m_pPtr + 1);
  }
  EZ_ALWAYS_INLINE reverse_pointer_iterator operator--(int)
  {
    this->m_pPtr++;
    return reverse_pointer_iterator(this->m_pPtr - 1);
  }

  using const_reverse_pointer_iterator<T>::operator+;
  using const_reverse_pointer_iterator<T>::operator-;

  EZ_ALWAYS_INLINE reverse_pointer_iterator operator+(ptrdiff_t rhs) const { return reverse_pointer_iterator(this->m_pPtr - rhs); }
  EZ_ALWAYS_INLINE reverse_pointer_iterator operator-(ptrdiff_t rhs) const { return reverse_pointer_iterator(this->m_pPtr + rhs); }

  EZ_ALWAYS_INLINE T& operator*() const { return *(this->m_pPtr); }
  EZ_ALWAYS_INLINE T* operator->() const { return this->m_pPtr; }
  EZ_ALWAYS_INLINE T& operator[](ptrdiff_t iIndex) const { return *(this->m_pPtr - iIndex); }
};
