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
  typedef ezDelegate<IteratorType()> BeginCallback;
  typedef ezDelegate<IteratorType()> EndCallback;
  typedef ezDelegate<void(IteratorType&)> NextCallback;
  typedef ezDelegate<ValueType(const IteratorType&)> ValueCallback;

  /// \brief Initializes the ezRangeView with the delegates used to enumerate the range.
  EZ_ALWAYS_INLINE ezRangeView(BeginCallback begin, EndCallback end, NextCallback next, ValueCallback value);

  /// \brief Const iterator, don't use directly, use ranged based for loops or call begin() end().
  struct ConstIterator
  {
    EZ_DECLARE_POD_TYPE();

    typedef std::forward_iterator_tag iterator_category;
    typedef ConstIterator value_type;
    typedef ConstIterator* pointer;
    typedef ConstIterator& reference;

    EZ_ALWAYS_INLINE ConstIterator(const ConstIterator& rhs) = default;
    EZ_FORCE_INLINE void Next();
    EZ_FORCE_INLINE const ValueType Value() const;
    EZ_ALWAYS_INLINE const ValueType operator*() const { return Value(); }
    EZ_ALWAYS_INLINE void operator++() { Next(); }
    EZ_FORCE_INLINE bool operator==(const typename ezRangeView<ValueType, IteratorType>::ConstIterator& it2) const;
    EZ_FORCE_INLINE bool operator!=(const typename ezRangeView<ValueType, IteratorType>::ConstIterator& it2) const;

  protected:
    EZ_FORCE_INLINE explicit ConstIterator(const ezRangeView<ValueType, IteratorType>* view, IteratorType pos);

    friend class ezRangeView<ValueType, IteratorType>;
    const ezRangeView<ValueType, IteratorType>* m_View = nullptr;
    IteratorType m_Pos;
  };

  /// \brief Iterator, don't use directly, use ranged based for loops or call begin() end().
  struct Iterator : public ConstIterator
  {
    EZ_DECLARE_POD_TYPE();

    typedef std::forward_iterator_tag iterator_category;
    typedef Iterator value_type;
    typedef Iterator* pointer;
    typedef Iterator& reference;

    using ConstIterator::Value;
    EZ_ALWAYS_INLINE Iterator(const Iterator& rhs) = default;
    EZ_FORCE_INLINE ValueType Value();
    EZ_ALWAYS_INLINE ValueType operator*() { return Value(); }

  protected:
    EZ_FORCE_INLINE explicit Iterator(const ezRangeView<ValueType, IteratorType>* view, IteratorType pos);
  };

  Iterator begin() { return Iterator(this, m_begin()); }
  Iterator end() { return Iterator(this, m_end()); }
  ConstIterator begin() const { return ConstIterator(this, m_begin()); }
  ConstIterator end() const { return ConstIterator(this, m_end()); }
  ConstIterator cbegin() const { return ConstIterator(this, m_begin()); }
  ConstIterator cend() const { return ConstIterator(this, m_end()); }

private:
  friend struct Iterator;
  friend struct ConstIterator;

  BeginCallback m_begin;
  EndCallback m_end;
  NextCallback m_next;
  ValueCallback m_value;
};

template <typename V, typename I>
typename ezRangeView<V, I>::Iterator begin(ezRangeView<V, I>& container)
{
  return container.begin();
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
typename ezRangeView<V, I>::Iterator end(ezRangeView<V, I>& container)
{
  return container.end();
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

