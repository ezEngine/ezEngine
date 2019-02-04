
template <typename Derived>
ezUInt32 ezHashHelperString_NoCase::Hash(const ezStringBase<Derived>& value)
{
  ezHybridArray<char, 256> temp;
  temp.SetCountUninitialized(value.InternalGetElementCount());
  ezMemoryUtils::Copy(temp.GetData(), value.InternalGetData(), value.InternalGetElementCount());
  const ezUInt32 uiElemCount = ezStringUtils::ToLowerString(temp.GetData(), temp.GetData() + value.InternalGetElementCount());
  return ezHashingUtils::MurmurHash32((void*)temp.GetData(), uiElemCount);
}

template <typename DerivedLhs, typename DerivedRhs>
bool ezHashHelperString_NoCase::Equal(const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs)
{
  return ezStringUtils::IsEqual_NoCase(lhs.InternalGetData(), rhs.InternalGetData(), lhs.InternalGetDataEnd(), rhs.InternalGetDataEnd());
}

template <typename DerivedLhs>
bool ezHashHelperString_NoCase::Equal(const ezStringBase<DerivedLhs>& lhs, const char* rhs)
{
  return lhs.IsEqual_NoCase(rhs);
}

