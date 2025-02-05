#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScriptPlugin/Runtime/AsStringFactory.h>

const void* ezAsStringFactory::GetStringConstant(const char* data, asUINT length)
{
  ezHashedString hs;
  hs.Assign(ezStringView(data, length));

  // we need to give out a pointer to a StringView that doesn't vanish
  EZ_LOCK(m_Mutex);
  auto it = m_Strings.Insert(hs.GetView());
  const ezStringView& view = it.Key();

  return &view;
}

int ezAsStringFactory::ReleaseStringConstant(const void* str)
{
  // we don't clean up the strings
  return 0;
}

int ezAsStringFactory::GetRawStringData(const void* str, char* data, asUINT* length) const
{
  const ezStringView* pView = (const ezStringView*)str;

  *length = pView->GetElementCount();

  if (data)
  {
    ezStringUtils::Copy(data, *length + 1, pView->GetStartPointer());
  }

  return 0;
}
