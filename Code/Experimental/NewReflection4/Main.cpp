#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>

#include "Main.h"
#include "RTTI.h"

EZ_IMPLEMENT_REFLECTABLE_TYPE(TestBase);
EZ_IMPLEMENT_REFLECTABLE_TYPE_WITH_BASE(TestClassA, TestBase);

void PrintTypeInfo(const ezRTTI* pRTTI)
{
  if (pRTTI == NULL)
    return;

  ezLog::Info("Type: '%s'", pRTTI->GetTypeName());

  if (pRTTI->GetParentType() != NULL)
  {
    EZ_LOG_BLOCK("Parent Type");
    PrintTypeInfo(pRTTI->GetParentType());
  }
}

int main()
{
  ezStartup::StartupCore();

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  PrintTypeInfo(ezGetStaticRTTI<float>());
  PrintTypeInfo(ezGetStaticRTTI<ezVec3>());
  PrintTypeInfo(ezGetStaticRTTI<int>());
  PrintTypeInfo(ezGetStaticRTTI<unsigned char>());
  PrintTypeInfo(ezGetStaticRTTI<const char*>());

  {
    EZ_LOG_BLOCK("All RTTI Types");

    ezRTTI* pRTTI = ezRTTI::GetFirstInstance();

    while (pRTTI)
    {
      PrintTypeInfo(pRTTI);

      pRTTI = pRTTI->GetNextInstance();
    }
  }

  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  ezStartup::ShutdownBase();
}