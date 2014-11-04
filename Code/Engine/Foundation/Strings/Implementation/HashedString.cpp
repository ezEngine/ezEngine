#include <Foundation/PCH.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Lock.h>

static ezHashedString::StringStorage g_HashedStrings;
static bool g_bHashedStringsInitialized = false;
static ezHashedString::HashedType g_hsEmpty;
static ezMutex g_HashedStringMutex;

// static
ezHashedString::HashedType ezHashedString::AddHashedString(const char* szString, ezUInt32 uiHash)
{
  EZ_LOCK(g_HashedStringMutex);

  ezString s(szString);

  // try to find the existing string
  auto ret = g_HashedStrings.Find(s);

  // if it already exists, just increase the refcount
  if (ret.IsValid())
    ret.Value().m_iRefCount.Increment();
  else
  {
    ezHashedString::HashedData d;
    d.m_iRefCount = 1;
    d.m_uiHash = uiHash;

    ret = g_HashedStrings.Insert(s, d); // otherwise insert it with a refcount of 1
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

  g_hsEmpty = AddHashedString("", ezHashing::MurmurHash(""));

  // this one should never get deleted, so make sure its refcount is 2
  g_hsEmpty.Value().m_iRefCount.Increment();
}

ezUInt32 ezHashedString::ClearUnusedStrings()
{
  EZ_LOCK(g_HashedStringMutex);

  ezUInt32 uiDeleted = 0;

  for (auto it = g_HashedStrings.GetIterator(); it.IsValid(); )
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

ezHashedString::ezHashedString()
{
  EZ_CHECK_AT_COMPILETIME_MSG(sizeof(m_Data) == sizeof(void*), "The hashed string data should only be as large as one pointer.");
  EZ_CHECK_AT_COMPILETIME_MSG(sizeof(*this) == sizeof(void*), "The hashed string data should only be as large as one pointer.");

  // only insert the empty string once, after that, we can just use it without the need for the mutex
  if (!g_bHashedStringsInitialized)
    InitHashedString();

  m_Data = g_hsEmpty;
}



EZ_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_HashedString);

