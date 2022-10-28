
template <typename ValueType, typename IteratorType>
ezRangeView<ValueType, IteratorType>::ezRangeView(BeginCallback begin, EndCallback end, NextCallback next, ValueCallback value)
  : m_Begin(begin)
  , m_End(end)
  , m_Next(next)
  , m_Value(value)
{
}

template <typename ValueType, typename IteratorType>
EZ_FORCE_INLINE void ezRangeView<ValueType, IteratorType>::ConstIterator::Next()
{
  this->m_pView->m_Next(this->m_Pos);
}

template <typename ValueType, typename IteratorType>
EZ_FORCE_INLINE ValueType ezRangeView<ValueType, IteratorType>::ConstIterator::Value() const
{
  return this->m_pView->m_Value(this->m_Pos);
}

template <typename ValueType, typename IteratorType>
EZ_FORCE_INLINE bool ezRangeView<ValueType, IteratorType>::ConstIterator::operator==(
  const typename ezRangeView<ValueType, IteratorType>::ConstIterator& it2) const
{
  return m_pView == it2.m_pView && m_Pos == it2.m_Pos;
}

template <typename ValueType, typename IteratorType>
EZ_FORCE_INLINE bool ezRangeView<ValueType, IteratorType>::ConstIterator::operator!=(
  const typename ezRangeView<ValueType, IteratorType>::ConstIterator& it2) const
{
  return !(*this == it2);
}

template <typename ValueType, typename IteratorType>
EZ_FORCE_INLINE ezRangeView<ValueType, IteratorType>::ConstIterator::ConstIterator(const ezRangeView<ValueType, IteratorType>* view, IteratorType pos)
{
  m_pView = view;
  m_Pos = pos;
}

template <typename ValueType, typename IteratorType>
EZ_FORCE_INLINE ValueType ezRangeView<ValueType, IteratorType>::Iterator::Value()
{
  return this->m_View->m_value(this->m_Pos);
}

template <typename ValueType, typename IteratorType>
ezRangeView<ValueType, IteratorType>::Iterator::Iterator(const ezRangeView<ValueType, IteratorType>* view, IteratorType pos)
  : ezRangeView<ValueType, IteratorType>::ConstIterator(view, pos)
{
}
