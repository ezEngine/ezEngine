#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>

#include "Main.h"
#include "RTTI.h"

EZ_BEGIN_REFLECTED_TYPE(TestBase, ezReflectedClass, ezRTTIDefaultAllocator<TestBase>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY(Test),
    EZ_MEMBER_PROPERTY(Test2)
  EZ_END_PROPERTIES
EZ_END_REFLECTED_TYPE(TestBase);

EZ_BEGIN_REFLECTED_TYPE(TestClassA, TestBase, ezRTTIDefaultAllocator<TestClassA>);
EZ_END_REFLECTED_TYPE(TestClassA);

void PrintTypeInfo(const ezRTTI* pRTTI, bool bAllocate = false, bool bRecursive = true)
{
  if (pRTTI == NULL)
    return;

  ezLog::Info("Type: '%s'", pRTTI->GetTypeName());

  if (pRTTI->GetAllocator()->CanAllocate())
  {
    ezLog::Dev("  Type can be allocated dynamically.");

    // do a test
    if (bAllocate)
    {
      void* pObject = pRTTI->GetAllocator()->Allocate();
      pRTTI->GetAllocator()->Deallocate(pObject);
    }
  }

  {
    for (ezUInt32 p = 0; p < pRTTI->GetPropertyCount(); ++p)
      ezLog::Info("  Property: '%s'", pRTTI->GetProperty(p)->GetPropertyName());
  }

  if (!bRecursive)
    return;

  if (pRTTI->GetParentType() != NULL)
  {
    EZ_LOG_BLOCK("Parent Type");
    PrintTypeInfo(pRTTI->GetParentType(), false, true);
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

  {
    EZ_LOG_BLOCK("All Dynamic RTTI Types");

    ezRTTI* pRTTI = ezRTTI::GetFirstInstance();

    while (pRTTI)
    {
      if (pRTTI->IsDerivedFrom<ezReflectedClass>())
        PrintTypeInfo(pRTTI, true, false);

      pRTTI = pRTTI->GetNextInstance();
    }
  }
  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  ezStartup::ShutdownBase();
}