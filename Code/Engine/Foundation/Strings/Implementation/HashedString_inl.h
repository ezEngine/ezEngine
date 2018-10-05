#pragma once

#include <Foundation/Algorithm/Hashing.h>

inline ezHashedString::ezHashedString(const ezHashedString& rhs)
{
  m_Data = rhs.m_Data;

  // the string has a refcount of at least one (rhs holds a reference), thus it will definitely not get deleted on some other thread
  // therefore we can simply increase the refcount without locking
  m_Data.Value().m_iRefCount.Increment();
}

inline ezHashedString::~ezHashedString()
{
  // just decrease the refcount of the object that we are set to, it might reach refcount zero, but we don't care about that here
  m_Data.Value().m_iRefCount.Decrement();
}

inline void ezHashedString::operator=(const ezHashedString& rhs)
{
  // first increase the other refcount, then decrease ours
  HashedType tmp = rhs.m_Data;
  tmp.Value().m_iRefCount.Increment();

  m_Data.Value().m_iRefCount.Decrement();

  m_Data = tmp;
}

template <size_t N>
EZ_FORCE_INLINE void ezHashedString::Assign(const char (&szString)[N])
{
  HashedType tmp = m_Data;

  // this function will already increase the refcount as needed
  m_Data = AddHashedString(szString, ezHashing::MurmurHash32String(szString));

  tmp.Value().m_iRefCount.Decrement();
}

EZ_FORCE_INLINE void ezHashedString::Assign(ezHashing::StringWrapper szString)
{
  HashedType tmp = m_Data;

  // this function will already increase the refcount as needed
  m_Data = AddHashedString(szString.m_str, ezHashing::MurmurHash32String(szString));

  tmp.Value().m_iRefCount.Decrement();
}

inline bool ezHashedString::operator==(const ezHashedString& rhs) const
{
  return m_Data == rhs.m_Data;
}

inline bool ezHashedString::operator!=(const ezHashedString& rhs) const
{
  return !(*this == rhs);
}

inline bool ezHashedString::operator==(const ezTempHashedString& rhs) const
{
  return m_Data.Key() == rhs.m_uiHash;
}

inline bool ezHashedString::operator!=(const ezTempHashedString& rhs) const
{
  return !(*this == rhs);
}

inline bool ezHashedString::operator<(const ezHashedString& rhs) const
{
  return m_Data.Key() < rhs.m_Data.Key();
}

inline bool ezHashedString::operator<(const ezTempHashedString& rhs) const
{
  return m_Data.Key() < rhs.m_uiHash;
}

EZ_ALWAYS_INLINE const ezString& ezHashedString::GetString() const
{
  return m_Data.Value().m_sString;
}

EZ_ALWAYS_INLINE const char* ezHashedString::GetData() const
{
  return m_Data.Value().m_sString.GetData();
}

EZ_ALWAYS_INLINE ezUInt32 ezHashedString::GetHash() const
{
  return m_Data.Key();
}

template <size_t N>
EZ_FORCE_INLINE ezHashedString ezMakeHashedString(const char (&szString)[N])
{
  ezHashedString sResult;
  sResult.Assign(szString);
  return sResult;
}

//////////////////////////////////////////////////////////////////////////

template <size_t N>
EZ_ALWAYS_INLINE ezTempHashedString::ezTempHashedString(const char (&szString)[N])
{
  m_uiHash = ezHashing::MurmurHash32String<N>(szString);
}

EZ_ALWAYS_INLINE ezTempHashedString::ezTempHashedString(ezHashing::StringWrapper szString)
{
  m_uiHash = ezHashing::MurmurHash32String(szString);
}

EZ_ALWAYS_INLINE ezTempHashedString::ezTempHashedString(const ezTempHashedString& rhs)
{
  m_uiHash = rhs.m_uiHash;
}

EZ_ALWAYS_INLINE ezTempHashedString::ezTempHashedString(const ezHashedString& rhs)
{
  m_uiHash = rhs.GetHash();
}

EZ_ALWAYS_INLINE ezTempHashedString::ezTempHashedString(ezUInt32 uiHash)
{
  m_uiHash = uiHash;
}

template <size_t N>
EZ_ALWAYS_INLINE void ezTempHashedString::operator=(const char (&szString)[N])
{
  m_uiHash = ezHashing::MurmurHash32String<N>(szString);
}

EZ_ALWAYS_INLINE void ezTempHashedString::operator=(ezHashing::StringWrapper szString)
{
  m_uiHash = ezHashing::MurmurHash32String(szString);
}

EZ_ALWAYS_INLINE void ezTempHashedString::operator=(const ezTempHashedString& rhs)
{
  m_uiHash = rhs.m_uiHash;
}

EZ_ALWAYS_INLINE void ezTempHashedString::operator=(const ezHashedString& rhs)
{
  m_uiHash = rhs.GetHash();
}

EZ_ALWAYS_INLINE bool ezTempHashedString::operator==(const ezTempHashedString& rhs) const
{
  return m_uiHash == rhs.m_uiHash;
}

EZ_ALWAYS_INLINE bool ezTempHashedString::operator!=(const ezTempHashedString& rhs) const
{
  return !(m_uiHash == rhs.m_uiHash);
}

EZ_ALWAYS_INLINE bool ezTempHashedString::operator<(const ezTempHashedString& rhs) const
{
  return m_uiHash < rhs.m_uiHash;
}

EZ_ALWAYS_INLINE ezUInt32 ezTempHashedString::GetHash() const
{
  return m_uiHash;
}

template <size_t N>
EZ_ALWAYS_INLINE constexpr ezUInt32 ezTempHashedString::ComputeHash(const char (&szString)[N])
{
  return ezHashing::MurmurHash32String<N>(szString);
}

EZ_ALWAYS_INLINE ezUInt32 ezTempHashedString::ComputeHash(ezHashing::StringWrapper szString)
{
  return ezHashing::MurmurHash32String(szString.m_str);
}

//////////////////////////////////////////////////////////////////////////

template <>
struct ezHashHelper<ezHashedString>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezHashedString& value) { return value.GetHash(); }

  EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezTempHashedString& value) { return value.GetHash(); }

  EZ_ALWAYS_INLINE static bool Equal(const ezHashedString& a, const ezHashedString& b) { return a == b; }

  EZ_ALWAYS_INLINE static bool Equal(const ezHashedString& a, const ezTempHashedString& b) { return a == b; }
};

template <>
struct ezHashHelper<ezTempHashedString>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezTempHashedString& value) { return value.GetHash(); }

  EZ_ALWAYS_INLINE static bool Equal(const ezTempHashedString& a, const ezTempHashedString& b) { return a == b; }
};
