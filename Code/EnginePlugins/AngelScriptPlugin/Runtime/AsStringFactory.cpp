#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScriptPlugin/Runtime/AsStringFactory.h>
#include <Foundation/Strings/HashedString.h>

const void* ezAsStringFactory::GetStringConstant(const char* szData, asUINT length)
{
  ezHashedString hs;
  hs.Assign(ezStringView(szData, length));

  // we need to give out a pointer to a StringView that doesn't vanish
  EZ_LOCK(m_Mutex);
  auto it = m_Strings.Insert(hs.GetView());
  const ezStringView& view = it.Key();

  return &view;
}

int ezAsStringFactory::ReleaseStringConstant(const void* pStr)
{
  // we don't clean up the strings
  return 0;
}

int ezAsStringFactory::GetRawStringData(const void* pStr, char* szData, asUINT* pLength) const
{
  const ezStringView* pView = (const ezStringView*)pStr;

  *pLength = pView->GetElementCount();

  if (szData)
  {
    ezStringUtils::Copy(szData, *pLength + 1, pView->GetStartPointer());
  }

  return 0;
}
