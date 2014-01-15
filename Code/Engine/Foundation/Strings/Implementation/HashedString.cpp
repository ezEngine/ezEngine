#include <Foundation/PCH.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Algorithm/Hashing.h>

static ezHashedString::StringStorage g_HashedStrings;
static bool g_bHashedStringsInitialized = false;
static ezHashedString::HashedType g_hsEmpty;
static ezMutex g_HashedStringMutex;

static ezHashedString::HashedType AddHashedString(const char* szString)
{
  ezLock<ezMutex> l(g_HashedStringMutex);

  ezHashedString::StringType s(szString);

  // try to find the existing string
  auto ret = g_HashedStrings.Find(s);

  // if it already exists, just increase the refcount
  if (ret.IsValid())
    ret.Value().m_iRefCount.Increment();
  else
  {
    ezHashedString::HashedData d;
    d.m_iRefCount = 1;
    d.m_uiHash = ezHashing::MurmurHash((void*) szString, ezStringUtils::GetStringElementCount(szString));

    ret = g_HashedStrings.Insert(s, d); // otherwise insert it with a refcount of 1
  }

  return ret;
}

static void InitHashedString()
{
  // makes sure the empty string exists for the default constructor to use

  ezLock<ezMutex> l(g_HashedStringMutex);

  if (g_bHashedStringsInitialized)
    return;

  g_bHashedStringsInitialized = true;

  g_hsEmpty = AddHashedString("");

  // this one should never get deleted, so make sure its refcount is 2
  g_hsEmpty.Value().m_iRefCount.Increment();
}

ezUInt32 ezHashedString::ClearUnusedStrings()
{
  ezLock<ezMutex> l(g_HashedStringMutex);

  ezUInt32 uiDeleted = 0;

  for (auto it = g_HashedStrings.GetIterator(); it.IsValid(); )
  {
    if (it.Value().m_iRefCount == 0)
    {
      it = g_HashedStrings.Erase(it);
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

void ezHashedString::Assign(const char* szString)
{
  // just decrease the refcount of the object that we are set to, it might reach refcount zero, but we don't care about that here
  m_Data.Value().m_iRefCount.Decrement();

  // this function will already increase the refcount as needed
  m_Data = AddHashedString(szString);
}



EZ_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_HashedString);

