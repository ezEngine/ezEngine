#pragma once

#include <Foundation/Application/Application.h>

class ezStreamWriter;

class ezNoiseGen : public ezApplication
{
public:
  typedef ezApplication SUPER;

  struct KeyEnumValuePair
  {
    KeyEnumValuePair(const char* szKey, ezInt32 iVal)
      : m_szKey(szKey)
      , m_iEnumValue(iVal)
    {
    }

    const char* m_szKey;
    ezInt32 m_iEnumValue = -1;
  };

  ezNoiseGen();

public:
  virtual Execution Run() override;
  virtual ezResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;

private:
  ezString m_sOutputFile;
};
