#pragma once

#include <Foundation/Types/Delegate.h>

/// \brief This class uses delegates to define a range of values that can be enumerated using a forward iterator.
///
/// Can be used to create a contiguous view to elements of a certain type without the need for them to actually
/// exist in the same space or format. Think of IEnumerable in c# using composition via ezDelegate instead of derivation.
/// ValueType defines the value type we are iterating over and IteratorType is the internal key to identify an element.
/// An example that creates a RangeView of strings that are stored in a linear array of structs.
/// \code{.cpp}
/// auto range = ezRangeView<const char*, ezUInt32>(
///   [this]()-> ezUInt32 { return 0; },
///   [this]()-> ezUInt32 { return array.GetCount(); },
///   [this](ezUInt32& it) { ++it; },
///   [this](const ezUInt32& it)-> const char* { return array[it].m_String; });
///
/// for (const char* szValue : range)
/// {
/// }
/// \endcode
template <typename ValueType, typename IteratorType>
class ezRangeView
{
public:
  using BeginCallback = ezDelegate<IteratorType ()>;
  using EndCallback = ezDelegate<IteratorType ()>;
  using NextCallback = ezDelegate<void (IteratorType &)>;
  using ValueCallback = ezDelegate<ValueType (const IteratorType &)>;

  /// \brief Initializes the ezRangeView with the delegates used to enumerate the range.
  EZ_ALWAYS_INLINE ezRangeView(BeginCallback begin, EndCallback end, NextCallback next, ValueCallback value);

  /// \brief Const iterator, don't use directly, use ranged based for loops or call begin() end().
  struct ConstIterator
  {
    EZ_DECLARE_POD_TYPE();

    using iterator_category = std::forward_iterator_tag;
    using value_type = ConstIterator;
    using pointer = ConstIterator *;
    using reference = ConstIterator &;

    EZ_ALWAYS_INLINE ConstIterator(const ConstIterator& rhs) = default;
    EZ_FORCE_INLINE void Next();
    EZ_FORCE_INLINE ValueType Value() const;
    EZ_ALWAYS_INLINE ValueType operator*() const { return Value(); }
    EZ_ALWAYS_INLINE void operator++() { Next(); }
    EZ_FORCE_INLINE bool operator==(const typename ezRangeView<ValueType, IteratorType>::ConstIterator& it2) const;
    EZ_FORCE_INLINE bool operator!=(const typename ezRangeView<ValueType, IteratorType>::ConstIterator& it2) const;

  protected:
    EZ_FORCE_INLINE explicit ConstIterator(const ezRangeView<ValueType, IteratorType>* view, IteratorType pos);

    friend class ezRangeView<ValueType, IteratorType>;
    const ezRangeView<ValueType, IteratorType>* m_pView = nullptr;
    IteratorType m_Pos;
  };

  /// \brief Iterator, don't use directly, use ranged based for loops or call begin() end().
  struct Iterator : public ConstIterator
  {
    EZ_DECLARE_POD_TYPE();

    using iterator_category = std::forward_iterator_tag;
    using value_type = Iterator;
    using pointer = Iterator *;
    using reference = Iterator &;

    using ConstIterator::Value;
    EZ_ALWAYS_INLINE Iterator(const Iterator& rhs) = default;
    EZ_FORCE_INLINE ValueType Value();
    EZ_ALWAYS_INLINE ValueType operator*() { return Value(); }

  protected:
    EZ_FORCE_INLINE explicit Iterator(const ezRangeView<ValueType, IteratorType>* view, IteratorType pos);
  };

  Iterator begin() { return Iterator(this, m_Begin()); }
  Iterator end() { return Iterator(this, m_End()); }
  ConstIterator begin() const { return ConstIterator(this, m_Begin()); }
  ConstIterator end() const { return ConstIterator(this, m_End()); }
  ConstIterator cbegin() const { return ConstIterator(this, m_Begin()); }
  ConstIterator cend() const { return ConstIterator(this, m_End()); }

private:
  friend struct Iterator;
  friend struct ConstIterator;

  BeginCallback m_Begin;
  EndCallback m_End;
  NextCallback m_Next;
  ValueCallback m_Value;
};

template <typename V, typename I>
typename ezRangeView<V, I>::Iterator begin(ezRangeView<V, I>& in_container)
{
  return in_container.begin();
}

template <typename V, typename I>
typename ezRangeView<V, I>::ConstIterator begin(const ezRangeView<V, I>& container)
{
  return container.cbegin();
}

template <typename V, typename I>
typename ezRangeView<V, I>::ConstIterator cbegin(const ezRangeView<V, I>& container)
{
  return container.cbegin();
}

template <typename V, typename I>
typename ezRangeView<V, I>::Iterator end(ezRangeView<V, I>& in_container)
{
  return in_container.end();
}

template <typename V, typename I>
typename ezRangeView<V, I>::ConstIterator end(const ezRangeView<V, I>& container)
{
  return container.cend();
}

template <typename V, typename I>
typename ezRangeView<V, I>::ConstIterator cend(const ezRangeView<V, I>& container)
{
  return container.cend();
}

#include <Foundation/Types/Implementation/RangeView_inl.h>
