#include <FoundationPCH.h>

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

static ezHashedString::StringStorage g_HashedStrings;
static bool g_bHashedStringsInitialized = false;
static ezHashedString::HashedType g_hsEmpty;
static ezMutex g_HashedStringMutex;

// static
ezHashedString::HashedType ezHashedString::AddHashedString(const char* szString, ezUInt32 uiHash)
{
  EZ_LOCK(g_HashedStringMutex);

  // try to find the existing string
  bool bExisted = false;
  auto ret = g_HashedStrings.FindOrAdd(uiHash, &bExisted);

  // if it already exists, just increase the refcount
  if (bExisted)
  {
#if EZ_ENABLED(EZ_HASHED_STRING_REF_COUNTING)
    ret.Value().m_iRefCount.Increment();
#endif
  }
  else
  {
    ezHashedString::HashedData& d = ret.Value();
#if EZ_ENABLED(EZ_HASHED_STRING_REF_COUNTING)
    d.m_iRefCount = 1;
#endif
    d.m_sString = szString;
  }

  return ret;
}

// static
void ezHashedString::InitHashedString()
{
  // makes sure the empty string exists for the default constructor to use

  EZ_LOCK(g_HashedStringMutex);

  if (g_bHashedStringsInitialized)
    return;

  g_bHashedStringsInitialized = true;

  g_hsEmpty = AddHashedString("", ezHashingUtils::MurmurHash32String(""));

#if EZ_ENABLED(EZ_HASHED_STRING_REF_COUNTING)
  // this one should never get deleted, so make sure its refcount is 2
  g_hsEmpty.Value().m_iRefCount.Increment();
#endif
}

#if EZ_ENABLED(EZ_HASHED_STRING_REF_COUNTING)
ezUInt32 ezHashedString::ClearUnusedStrings()
{
  EZ_LOCK(g_HashedStringMutex);

  ezUInt32 uiDeleted = 0;

  for (auto it = g_HashedStrings.GetIterator(); it.IsValid();)
  {
    if (it.Value().m_iRefCount == 0)
    {
      it = g_HashedStrings.Remove(it);
      ++uiDeleted;
    }
    else
      ++it;
  }

  return uiDeleted;
}
#endif

ezHashedString::ezHashedString()
{
  EZ_CHECK_AT_COMPILETIME_MSG(sizeof(m_Data) == sizeof(void*), "The hashed string data should only be as large as one pointer.");
  EZ_CHECK_AT_COMPILETIME_MSG(sizeof(*this) == sizeof(void*), "The hashed string data should only be as large as one pointer.");

  // only insert the empty string once, after that, we can just use it without the need for the mutex
  if (!g_bHashedStringsInitialized)
    InitHashedString();

  m_Data = g_hsEmpty;
#if EZ_ENABLED(EZ_HASHED_STRING_REF_COUNTING)
  m_Data.Value().m_iRefCount.Increment();
#endif
}

bool ezHashedString::IsEmpty() const
{
  return m_Data == g_hsEmpty;
}

void ezHashedString::Clear()
{
#if EZ_ENABLED(EZ_HASHED_STRING_REF_COUNTING)
  if (m_Data != g_hsEmpty)
  {
    HashedType tmp = m_Data;

    m_Data = g_hsEmpty;
    m_Data.Value().m_iRefCount.Increment();

    tmp.Value().m_iRefCount.Decrement();
  }
#else
  m_Data = g_hsEmpty;
#endif  
}

EZ_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_HashedString);

