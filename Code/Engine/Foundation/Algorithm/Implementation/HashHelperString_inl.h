
ezUInt32 ezHashHelperString_NoCase::Hash(ezStringView value)
{
  ezHybridArray<char, 256> temp;
  temp.SetCountUninitialized(value.GetElementCount());
  ezMemoryUtils::Copy(temp.GetData(), value.GetStartPointer(), value.GetElementCount());
  const ezUInt32 uiElemCount = ezStringUtils::ToLowerString(temp.GetData(), temp.GetData() + value.GetElementCount());

  return ezHashingUtils::StringHashTo32(ezHashingUtils::xxHash64((void*)temp.GetData(), uiElemCount));
}

bool ezHashHelperString_NoCase::Equal(ezStringView lhs, ezStringView rhs)
{
  return lhs.IsEqual_NoCase(rhs);
}
