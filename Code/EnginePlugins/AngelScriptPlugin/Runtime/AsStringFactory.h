#pragma once

#include <AngelScriptPlugin/AngelScriptPluginDLL.h>

#include <AngelScript/include/angelscript.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/StringView.h>
#include <Foundation/Threading/Mutex.h>

class ezAsStringFactory : public asIStringFactory
{
public:
  ezAsStringFactory() = default;
  ~ezAsStringFactory() = default;

  const void* GetStringConstant(const char* szData, asUINT length) override;
  int ReleaseStringConstant(const void* pStr) override;
  int GetRawStringData(const void* pStr, char* szData, asUINT* pLength) const override;

private:
  ezMutex m_Mutex;
  ezSet<ezStringView> m_Strings;
};
