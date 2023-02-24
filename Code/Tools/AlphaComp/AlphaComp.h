#pragma once

#include <Foundation/Application/Application.h>

class ezStreamWriter;

class ezAlphaComp : public ezApplication
{
public:
  typedef ezApplication SUPER;

  struct KeyEnumValuePair
  {
    KeyEnumValuePair(const char* key, ezInt32 val)
      : m_szKey(key)
      , m_iEnumValue(val)
    {
    }

    const char* m_szKey;
    ezInt32 m_iEnumValue = -1;
  };

  ezAlphaComp();

public:
  virtual Execution Run() override;
  virtual ezResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;
  
private:
};
