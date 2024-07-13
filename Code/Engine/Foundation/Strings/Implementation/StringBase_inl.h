#pragma once

template <typename Derived>
EZ_ALWAYS_INLINE const char* ezStringBase<Derived>::InternalGetData() const
{
  const Derived* pDerived = static_cast<const Derived*>(this);
  return pDerived->GetData();
}

template <typename Derived>
EZ_ALWAYS_INLINE const char* ezStringBase<Derived>::InternalGetDataEnd() const
{
  const Derived* pDerived = static_cast<const Derived*>(this);
  return pDerived->GetData() + pDerived->GetElementCount();
}

template <typename Derived>
EZ_ALWAYS_INLINE ezUInt32 ezStringBase<Derived>::InternalGetElementCount() const
{
  const Derived* pDerived = static_cast<const Derived*>(this);
  return pDerived->GetElementCount();
}

template <typename Derived>
EZ_ALWAYS_INLINE bool ezStringBase<Derived>::IsEmpty() const
{
  return ezStringUtils::IsNullOrEmpty(InternalGetData()) || (InternalGetData() == InternalGetDataEnd());
}

template <typename Derived>
bool ezStringBase<Derived>::StartsWith(ezStringView sStartsWith) const
{
  return ezStringUtils::StartsWith(InternalGetData(), sStartsWith.GetStartPointer(), InternalGetDataEnd(), sStartsWith.GetEndPointer());
}

template <typename Derived>
bool ezStringBase<Derived>::StartsWith_NoCase(ezStringView sStartsWith) const
{
  return ezStringUtils::StartsWith_NoCase(InternalGetData(), sStartsWith.GetStartPointer(), InternalGetDataEnd(), sStartsWith.GetEndPointer());
}

template <typename Derived>
bool ezStringBase<Derived>::EndsWith(ezStringView sEndsWith) const
{
  return ezStringUtils::EndsWith(InternalGetData(), sEndsWith.GetStartPointer(), InternalGetDataEnd(), sEndsWith.GetEndPointer());
}

template <typename Derived>
bool ezStringBase<Derived>::EndsWith_NoCase(ezStringView sEndsWith) const
{
  return ezStringUtils::EndsWith_NoCase(InternalGetData(), sEndsWith.GetStartPointer(), InternalGetDataEnd(), sEndsWith.GetEndPointer());
}

template <typename Derived>
const char* ezStringBase<Derived>::FindSubString(ezStringView sStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetData();

  EZ_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return ezStringUtils::FindSubString(szStartSearchAt, sStringToFind.GetStartPointer(), InternalGetDataEnd(), sStringToFind.GetEndPointer());
}

template <typename Derived>
const char* ezStringBase<Derived>::FindSubString_NoCase(ezStringView sStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetData();

  EZ_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return ezStringUtils::FindSubString_NoCase(szStartSearchAt, sStringToFind.GetStartPointer(), InternalGetDataEnd(), sStringToFind.GetEndPointer());
}

template <typename Derived>
inline const char* ezStringBase<Derived>::FindLastSubString(ezStringView sStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetDataEnd();

  EZ_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()),
    "The given pointer to start searching at is not inside this strings valid range.");

  return ezStringUtils::FindLastSubString(InternalGetData(), sStringToFind.GetStartPointer(), szStartSearchAt, InternalGetDataEnd(), sStringToFind.GetEndPointer());
}

template <typename Derived>
inline const char* ezStringBase<Derived>::FindLastSubString_NoCase(ezStringView sStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetDataEnd();

  EZ_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()),
    "The given pointer to start searching at is not inside this strings valid range.");

  return ezStringUtils::FindLastSubString_NoCase(InternalGetData(), sStringToFind.GetStartPointer(), szStartSearchAt, InternalGetDataEnd(), sStringToFind.GetEndPointer());
}

template <typename Derived>
inline const char* ezStringBase<Derived>::FindWholeWord(const char* szSearchFor, ezStringUtils::EZ_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetData();

  EZ_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return ezStringUtils::FindWholeWord(szStartSearchAt, szSearchFor, isDelimiterCB, InternalGetDataEnd());
}

template <typename Derived>
inline const char* ezStringBase<Derived>::FindWholeWord_NoCase(const char* szSearchFor, ezStringUtils::EZ_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetData();

  EZ_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return ezStringUtils::FindWholeWord_NoCase(szStartSearchAt, szSearchFor, isDelimiterCB, InternalGetDataEnd());
}

template <typename Derived>
ezInt32 ezStringBase<Derived>::Compare(ezStringView sOther) const
{
  return ezStringUtils::Compare(InternalGetData(), sOther.GetStartPointer(), InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
ezInt32 ezStringBase<Derived>::CompareN(ezStringView sOther, ezUInt32 uiCharsToCompare) const
{
  return ezStringUtils::CompareN(InternalGetData(), sOther.GetStartPointer(), uiCharsToCompare, InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
ezInt32 ezStringBase<Derived>::Compare_NoCase(ezStringView sOther) const
{
  return ezStringUtils::Compare_NoCase(InternalGetData(), sOther.GetStartPointer(), InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
ezInt32 ezStringBase<Derived>::CompareN_NoCase(ezStringView sOther, ezUInt32 uiCharsToCompare) const
{
  return ezStringUtils::CompareN_NoCase(InternalGetData(), sOther.GetStartPointer(), uiCharsToCompare, InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
bool ezStringBase<Derived>::IsEqual(ezStringView sOther) const
{
  return ezStringUtils::IsEqual(InternalGetData(), sOther.GetStartPointer(), InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
bool ezStringBase<Derived>::IsEqualN(ezStringView sOther, ezUInt32 uiCharsToCompare) const
{
  return ezStringUtils::IsEqualN(InternalGetData(), sOther.GetStartPointer(), uiCharsToCompare, InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
bool ezStringBase<Derived>::IsEqual_NoCase(ezStringView sOther) const
{
  return ezStringUtils::IsEqual_NoCase(InternalGetData(), sOther.GetStartPointer(), InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
bool ezStringBase<Derived>::IsEqualN_NoCase(ezStringView sOther, ezUInt32 uiCharsToCompare) const
{
  return ezStringUtils::IsEqualN_NoCase(InternalGetData(), sOther.GetStartPointer(), uiCharsToCompare, InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
const char* ezStringBase<Derived>::ComputeCharacterPosition(ezUInt32 uiCharacterIndex) const
{
  const char* pos = InternalGetData();
  if (ezUnicodeUtils::MoveToNextUtf8(pos, InternalGetDataEnd(), uiCharacterIndex).Failed())
    return nullptr;

  return pos;
}

template <typename Derived>
typename ezStringBase<Derived>::iterator ezStringBase<Derived>::GetIteratorFront() const
{
  return begin(*this);
}

template <typename Derived>
typename ezStringBase<Derived>::reverse_iterator ezStringBase<Derived>::GetIteratorBack() const
{
  return rbegin(*this);
}

template <typename DerivedLhs, typename DerivedRhs>
EZ_ALWAYS_INLINE bool operator==(const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return lhs.IsEqual(rhs.GetView());
}

template <typename DerivedRhs>
EZ_ALWAYS_INLINE bool operator==(const char* lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.IsEqual(lhs);
}

template <typename DerivedLhs>
EZ_ALWAYS_INLINE bool operator==(const ezStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.IsEqual(rhs);
}

#if EZ_DISABLED(EZ_USE_CPP20_OPERATORS)

template <typename DerivedLhs, typename DerivedRhs>
EZ_ALWAYS_INLINE bool operator!=(const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return !lhs.IsEqual(rhs);
}

template <typename DerivedRhs>
EZ_ALWAYS_INLINE bool operator!=(const char* lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return !rhs.IsEqual(lhs);
}

template <typename DerivedLhs>
EZ_ALWAYS_INLINE bool operator!=(const ezStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return !lhs.IsEqual(rhs);
}

#endif

#if EZ_ENABLED(EZ_USE_CPP20_OPERATORS)

template <typename DerivedLhs, typename DerivedRhs>
EZ_ALWAYS_INLINE std::strong_ordering operator<=>(const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs)
{
  return lhs.Compare(rhs) <=> 0;
}

template <typename DerivedLhs, typename DerivedRhs>
EZ_ALWAYS_INLINE std::strong_ordering operator<=>(const ezStringBase<DerivedLhs>& lhs, const char* rhs)
{
  return lhs.Compare(rhs) <=> 0;
}

#else

template <typename DerivedLhs, typename DerivedRhs>
EZ_ALWAYS_INLINE bool operator<(const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return lhs.Compare(rhs) < 0;
}

template <typename DerivedRhs>
EZ_ALWAYS_INLINE bool operator<(const char* lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.Compare(lhs) > 0;
}

template <typename DerivedLhs>
EZ_ALWAYS_INLINE bool operator<(const ezStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.Compare(rhs) < 0;
}

template <typename DerivedLhs, typename DerivedRhs>
EZ_ALWAYS_INLINE bool operator>(const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return lhs.Compare(rhs) > 0;
}

template <typename DerivedRhs>
EZ_ALWAYS_INLINE bool operator>(const char* lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.Compare(lhs) < 0;
}

template <typename DerivedLhs>
EZ_ALWAYS_INLINE bool operator>(const ezStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.Compare(rhs) > 0;
}

template <typename DerivedLhs, typename DerivedRhs>
EZ_ALWAYS_INLINE bool operator<=(const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return ezStringUtils::Compare(lhs.InternalGetData(), rhs.InternalGetData(), lhs.InternalGetDataEnd(), rhs.InternalGetDataEnd()) <= 0;
}

template <typename DerivedRhs>
EZ_ALWAYS_INLINE bool operator<=(const char* lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.Compare(lhs) >= 0;
}

template <typename DerivedLhs>
EZ_ALWAYS_INLINE bool operator<=(const ezStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.Compare(rhs) <= 0;
}

template <typename DerivedLhs, typename DerivedRhs>
EZ_ALWAYS_INLINE bool operator>=(const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return ezStringUtils::Compare(lhs.InternalGetData(), rhs.InternalGetData(), lhs.InternalGetDataEnd(), rhs.InternalGetDataEnd()) >= 0;
}

template <typename DerivedRhs>
EZ_ALWAYS_INLINE bool operator>=(const char* lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.Compare(lhs) <= 0;
}

template <typename DerivedLhs>
EZ_ALWAYS_INLINE bool operator>=(const ezStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.Compare(rhs) >= 0;
}

#endif

template <typename DerivedLhs>
EZ_ALWAYS_INLINE ezStringBase<DerivedLhs>::operator ezStringView() const
{
  return ezStringView(InternalGetData(), InternalGetElementCount());
}

template <typename Derived>
EZ_ALWAYS_INLINE ezStringView ezStringBase<Derived>::GetView() const
{
  return ezStringView(InternalGetData(), InternalGetElementCount());
}

template <typename Derived>
template <typename Container>
void ezStringBase<Derived>::Split(bool bReturnEmptyStrings, Container& ref_output, const char* szSeparator1, const char* szSeparator2 /*= nullptr*/, const char* szSeparator3 /*= nullptr*/, const char* szSeparator4 /*= nullptr*/, const char* szSeparator5 /*= nullptr*/, const char* szSeparator6 /*= nullptr*/) const
{
  GetView().Split(bReturnEmptyStrings, ref_output, szSeparator1, szSeparator2, szSeparator3, szSeparator4, szSeparator5, szSeparator6);
}

template <typename Derived>
ezStringView ezStringBase<Derived>::GetRootedPathRootName() const
{
  return GetView().GetRootedPathRootName();
}

template <typename Derived>
bool ezStringBase<Derived>::IsRootedPath() const
{
  return GetView().IsRootedPath();
}

template <typename Derived>
bool ezStringBase<Derived>::IsRelativePath() const
{
  return GetView().IsRelativePath();
}

template <typename Derived>
bool ezStringBase<Derived>::IsAbsolutePath() const
{
  return GetView().IsAbsolutePath();
}

template <typename Derived>
ezStringView ezStringBase<Derived>::GetFileDirectory() const
{
  return GetView().GetFileDirectory();
}

template <typename Derived>
ezStringView ezStringBase<Derived>::GetFileNameAndExtension() const
{
  return GetView().GetFileNameAndExtension();
}

template <typename Derived>
ezStringView ezStringBase<Derived>::GetFileName() const
{
  return GetView().GetFileName();
}

template <typename Derived>
ezStringView ezStringBase<Derived>::GetFileExtension(bool bFullExtension) const
{
  return GetView().GetFileExtension(bFullExtension);
}

template <typename Derived>
bool ezStringBase<Derived>::HasExtension(ezStringView sExtension) const
{
  return GetView().HasExtension(sExtension);
}

template <typename Derived>
bool ezStringBase<Derived>::HasAnyExtension() const
{
  return GetView().HasAnyExtension();
}
