#pragma once

#include <AngelScriptPlugin/AngelScriptPluginDLL.h>

#include <AngelScript/include/angelscript.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/StringView.h>
#include <Foundation/Threading/Mutex.h>

class ezAsStringFactory : public asIStringFactory
{
public:
  ezAsStringFactory();
  ~ezAsStringFactory();

  const void* GetStringConstant(const char* szData, asUINT length) override;
  int ReleaseStringConstant(const void* pStr) override;
  int GetRawStringData(const void* pStr, char* szData, asUINT* pLength) const override;

  static ezAsStringFactory* GetFactory() { return s_pFactory; }

  const ezString& StoreString(const ezString& sStr);

private:
  static ezAsStringFactory* s_pFactory;
  ezMutex m_Mutex;
  ezSet<ezString> m_Strings;
  ezSet<ezStringView> m_StringViews;
};
