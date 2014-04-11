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

inline void ezHashedString::operator= (const ezHashedString& rhs)
{
  // just decrease the refcount of the object that we are set to, it might reach refcount zero, but we don't care about that here
  m_Data.Value().m_iRefCount.Decrement();

  m_Data = rhs.m_Data;
  m_Data.Value().m_iRefCount.Increment();
}

inline bool ezHashedString::operator== (const ezHashedString& rhs) const
{
  return &m_Data.Key() == &rhs.m_Data.Key();
}

inline bool ezHashedString::operator!= (const ezHashedString& rhs) const
{
  return !(*this == rhs);
}

inline bool ezHashedString::operator== (const ezTempHashedString& rhs) const
{
  return m_Data.Value().m_uiHash == rhs.m_uiHash;
}

inline bool ezHashedString::operator!= (const ezTempHashedString& rhs) const
{
  return !(*this == rhs);
}

inline bool ezHashedString::operator< (const ezHashedString& rhs) const
{
  return m_Data.Value().m_uiHash < rhs.m_Data.Value().m_uiHash;
}

inline bool ezHashedString::operator< (const ezTempHashedString& rhs) const
{
  return m_Data.Value().m_uiHash < rhs.m_uiHash;
}

inline const ezString& ezHashedString::GetString() const
{
  return m_Data.Key();
}

inline ezUInt32 ezHashedString::GetStringHash() const
{
  return m_Data.Value().m_uiHash;
}

inline ezTempHashedString::ezTempHashedString(const char* szString)
{
  m_uiHash = ezHashing::MurmurHash((void*) szString, ezStringUtils::GetStringElementCount(szString));
}

inline ezTempHashedString::ezTempHashedString(const ezTempHashedString& rhs)
{
  m_uiHash = rhs.m_uiHash;
}

inline ezTempHashedString::ezTempHashedString(const ezHashedString& rhs)
{
  /// \test this is new
  m_uiHash = rhs.GetStringHash();
}

inline void ezTempHashedString::operator= (const ezTempHashedString& rhs)
{
  m_uiHash = rhs.m_uiHash;
}

inline void ezTempHashedString::operator= (const ezHashedString& rhs)
{
  /// \test this is new
  m_uiHash = rhs.GetStringHash();
}

inline bool ezTempHashedString::operator==  (const ezTempHashedString& rhs) const
{
  return m_uiHash == rhs.m_uiHash;
}

inline bool ezTempHashedString::operator!=  (const ezTempHashedString& rhs) const
{
  return !(m_uiHash == rhs.m_uiHash);
}

inline bool ezTempHashedString::operator< (const ezTempHashedString& rhs) const
{
  return m_uiHash < rhs.m_uiHash;
}


