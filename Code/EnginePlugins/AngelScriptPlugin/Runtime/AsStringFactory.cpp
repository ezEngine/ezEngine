#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScriptPlugin/Runtime/AsStringFactory.h>
#include <Foundation/Strings/HashedString.h>

ezAsStringFactory* ezAsStringFactory::s_pFactory = nullptr;

ezAsStringFactory::ezAsStringFactory()
{
  s_pFactory = this;
}

ezAsStringFactory::~ezAsStringFactory()
{
  s_pFactory = nullptr;
}

const void* ezAsStringFactory::GetStringConstant(const char* szData, asUINT length)
{
  const ezString str(ezStringView(szData, length));

  // we need to give out a pointer to a StringView that doesn't vanish
  EZ_LOCK(m_Mutex);
  auto itStr = m_Strings.Insert(str);
  auto itView = m_StringViews.Insert(itStr.Key().GetView());
  const ezStringView& view = itView.Key();

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

const ezString& ezAsStringFactory::StoreString(const ezString& sStr)
{
  EZ_LOCK(m_Mutex);
  auto itStr = m_Strings.Insert(sStr);

  return itStr.Key();
}
