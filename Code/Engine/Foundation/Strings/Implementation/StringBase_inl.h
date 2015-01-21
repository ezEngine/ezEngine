#pragma once

template <typename Derived>
EZ_FORCE_INLINE const char* ezStringBase<Derived>::InternalGetData() const
{
  const Derived* pDerived = static_cast<const Derived*>(this);
  return pDerived->GetData();
}

template <typename Derived>
EZ_FORCE_INLINE const char* ezStringBase<Derived>::InternalGetDataEnd() const
{
  const Derived* pDerived = static_cast<const Derived*>(this);
  return pDerived->GetData() + pDerived->GetElementCount();
}

template <typename Derived>
EZ_FORCE_INLINE ezUInt32 ezStringBase<Derived>::InternalGetElementCount() const
{
  const Derived* pDerived = static_cast<const Derived*>(this);
  return pDerived->GetElementCount();
}

template <typename Derived>
EZ_FORCE_INLINE bool ezStringBase<Derived>::IsEmpty() const
{
  return ezStringUtils::IsNullOrEmpty(InternalGetData()) || (InternalGetData() == InternalGetDataEnd());
}

template <typename Derived>
bool ezStringBase<Derived>::StartsWith(const char* szStartsWith) const
{
  return ezStringUtils::StartsWith(InternalGetData(), szStartsWith, InternalGetDataEnd());
}

template <typename Derived>
bool ezStringBase<Derived>::StartsWith_NoCase(const char* szStartsWith) const
{
  return ezStringUtils::StartsWith_NoCase(InternalGetData(), szStartsWith, InternalGetDataEnd());
}

template <typename Derived>
bool ezStringBase<Derived>::EndsWith(const char* szEndsWith) const
{
  return ezStringUtils::EndsWith(InternalGetData(), szEndsWith, InternalGetDataEnd());
}

template <typename Derived>
bool ezStringBase<Derived>::EndsWith_NoCase(const char* szEndsWith) const
{
  return ezStringUtils::EndsWith_NoCase(InternalGetData(), szEndsWith, InternalGetDataEnd());
}

template <typename Derived>
const char* ezStringBase<Derived>::FindSubString(const char* szStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetData();

  EZ_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return ezStringUtils::FindSubString(szStartSearchAt, szStringToFind, InternalGetDataEnd());
}

template <typename Derived>
const char* ezStringBase<Derived>::FindSubString_NoCase(const char* szStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetData();

  EZ_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return ezStringUtils::FindSubString_NoCase(szStartSearchAt, szStringToFind, InternalGetDataEnd());
}

template <typename Derived>
inline const char* ezStringBase<Derived>::FindLastSubString(const char* szStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetDataEnd();

  EZ_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return ezStringUtils::FindLastSubString(InternalGetData(), szStringToFind, szStartSearchAt, InternalGetDataEnd());
}

template <typename Derived>
inline const char* ezStringBase<Derived>::FindLastSubString_NoCase(const char* szStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetDataEnd();

  EZ_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return ezStringUtils::FindLastSubString_NoCase(InternalGetData(), szStringToFind, szStartSearchAt, InternalGetDataEnd());
}

template <typename Derived>
inline const char* ezStringBase<Derived>::FindWholeWord(const char* szSearchFor, ezStringUtils::EZ_CHARACTER_FILTER IsDelimiterCB, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetData();

  EZ_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return ezStringUtils::FindWholeWord(szStartSearchAt, szSearchFor, IsDelimiterCB, InternalGetDataEnd());
}

template <typename Derived>
inline const char* ezStringBase<Derived>::FindWholeWord_NoCase(const char* szSearchFor, ezStringUtils::EZ_CHARACTER_FILTER IsDelimiterCB, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetData();

  EZ_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return ezStringUtils::FindWholeWord_NoCase(szStartSearchAt, szSearchFor, IsDelimiterCB, InternalGetDataEnd());
}

template <typename Derived>
ezInt32 ezStringBase<Derived>::Compare(const char* pString2) const
{
  return ezStringUtils::Compare(InternalGetData(), pString2, InternalGetDataEnd());
}

template <typename Derived>
ezInt32 ezStringBase<Derived>::CompareN(const char* pString2, ezUInt32 uiCharsToCompare) const
{
  return ezStringUtils::CompareN(InternalGetData(), pString2, uiCharsToCompare, InternalGetDataEnd());
}

template <typename Derived>
ezInt32 ezStringBase<Derived>::Compare_NoCase(const char* pString2) const
{
  return ezStringUtils::Compare_NoCase(InternalGetData(), pString2, InternalGetDataEnd());
}

template <typename Derived>
ezInt32 ezStringBase<Derived>::CompareN_NoCase(const char* pString2, ezUInt32 uiCharsToCompare) const
{
  return ezStringUtils::CompareN_NoCase(InternalGetData(), pString2, uiCharsToCompare, InternalGetDataEnd());
}

template <typename Derived>
bool ezStringBase<Derived>::IsEqual(const char* pString2) const
{
  return ezStringUtils::IsEqual(InternalGetData(), pString2, InternalGetDataEnd());
}

template <typename Derived>
bool ezStringBase<Derived>::IsEqualN(const char* pString2, ezUInt32 uiCharsToCompare) const
{
  return ezStringUtils::IsEqualN(InternalGetData(), pString2, uiCharsToCompare, InternalGetDataEnd());
}

template <typename Derived>
bool ezStringBase<Derived>::IsEqual_NoCase(const char* pString2) const
{
  return ezStringUtils::IsEqual_NoCase(InternalGetData(), pString2, InternalGetDataEnd());
}

template <typename Derived>
bool ezStringBase<Derived>::IsEqualN_NoCase(const char* pString2, ezUInt32 uiCharsToCompare) const
{
  return ezStringUtils::IsEqualN_NoCase(InternalGetData(), pString2, uiCharsToCompare, InternalGetDataEnd());
}

template <typename Derived>
const char* ezStringBase<Derived>::ComputeCharacterPosition(ezUInt32 uiCharacterIndex) const
{
  const char* pos = InternalGetData();
  ezUnicodeUtils::MoveToNextUtf8(pos, uiCharacterIndex);
  return pos;
}

template <typename DerivedLhs, typename DerivedRhs>
EZ_FORCE_INLINE bool operator== (const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return ezStringUtils::IsEqual(lhs.InternalGetData(), rhs.InternalGetData(), lhs.InternalGetDataEnd(), rhs.InternalGetDataEnd());
}

template <typename DerivedRhs>
EZ_FORCE_INLINE bool operator== (const char* lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.IsEqual(lhs);
}

template <typename DerivedLhs>
EZ_FORCE_INLINE bool operator== (const ezStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.IsEqual(rhs);
}

template <typename DerivedLhs, typename DerivedRhs>
EZ_FORCE_INLINE bool operator!= (const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return !ezStringUtils::IsEqual(lhs.InternalGetData(), rhs.InternalGetData(), lhs.InternalGetDataEnd(), rhs.InternalGetDataEnd());
}

template <typename DerivedRhs>
EZ_FORCE_INLINE bool operator!= (const char* lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return !rhs.IsEqual(lhs);
}

template <typename DerivedLhs>
EZ_FORCE_INLINE bool operator!= (const ezStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return !lhs.IsEqual(rhs);
}

template <typename DerivedLhs, typename DerivedRhs>
EZ_FORCE_INLINE bool operator< (const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return ezStringUtils::Compare(lhs.InternalGetData(), rhs.InternalGetData(), lhs.InternalGetDataEnd(), rhs.InternalGetDataEnd()) < 0;
}

template <typename DerivedRhs>
EZ_FORCE_INLINE bool operator< (const char* lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.Compare(lhs) > 0;
}

template <typename DerivedLhs>
EZ_FORCE_INLINE bool operator< (const ezStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.Compare(rhs) < 0;
}

template <typename DerivedLhs, typename DerivedRhs>
EZ_FORCE_INLINE bool operator> (const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return ezStringUtils::Compare(lhs.InternalGetData(), rhs.InternalGetData(), lhs.InternalGetDataEnd(), rhs.InternalGetDataEnd()) > 0;
}

template <typename DerivedRhs>
EZ_FORCE_INLINE bool operator> (const char* lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.Compare(lhs) < 0;
}

template <typename DerivedLhs>
EZ_FORCE_INLINE bool operator> (const ezStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.Compare(rhs) > 0;
}

template <typename DerivedLhs, typename DerivedRhs>
EZ_FORCE_INLINE bool operator<= (const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return ezStringUtils::Compare(lhs.InternalGetData(), rhs.InternalGetData(), lhs.InternalGetDataEnd(), rhs.InternalGetDataEnd()) <= 0;
}

template <typename DerivedRhs>
EZ_FORCE_INLINE bool operator<= (const char* lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.Compare(lhs) >= 0;
}

template <typename DerivedLhs>
EZ_FORCE_INLINE bool operator<= (const ezStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.Compare(rhs) <= 0;
}

template <typename DerivedLhs, typename DerivedRhs>
EZ_FORCE_INLINE bool operator>= (const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return ezStringUtils::Compare(lhs.InternalGetData(), rhs.InternalGetData(), lhs.InternalGetDataEnd(), rhs.InternalGetDataEnd()) >= 0;
}

template <typename DerivedRhs>
EZ_FORCE_INLINE bool operator>= (const char* lhs, const ezStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.Compare(lhs) <= 0;
}

template <typename DerivedLhs>
EZ_FORCE_INLINE bool operator>= (const ezStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.Compare(rhs) >= 0;
}


