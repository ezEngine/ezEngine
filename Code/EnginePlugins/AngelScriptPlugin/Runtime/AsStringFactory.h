#pragma once

#include <AngelScriptPlugin/AngelScriptPluginDLL.h>

#include <AngelScript/include/angelscript.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Threading/Mutex.h>

class ezAsStringFactory : public asIStringFactory
{
public:
  ezAsStringFactory() = default;
  ~ezAsStringFactory() = default;

  const void* GetStringConstant(const char* data, asUINT length) override;
  int ReleaseStringConstant(const void* str) override;
  int GetRawStringData(const void* str, char* data, asUINT* length) const override;

private:
  ezMutex m_Mutex;
  ezSet<ezStringView> m_Strings;
};
