#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>

#include "Test.h"

EZ_IMPLEMENT_REFLECTED_CLASS(TestBase);
EZ_IMPLEMENT_REFLECTED_CLASS(TestA);
EZ_IMPLEMENT_REFLECTED_CLASS(TestAB);
EZ_IMPLEMENT_REFLECTED_CLASS(TestB);

EZ_IMPLEMENT_REFLECTED_STRUCT(StructA);

void PrintStats(const ezTypeRTTI* pRTTI)
{
  const ezTypeRTTI* pParentRTTI = pRTTI->GetParentRTTI();

  EZ_LOG_BLOCK("Reflected Class", pRTTI->GetTypeName());

  ezLog::Info("Name: %s", pRTTI->GetTypeName());
  ezLog::Info("ID: %i", pRTTI->GetTypeID());
  ezLog::Info("Type: %s", pRTTI->GetTypeMode() == ezTypeRTTI::Class ? "class" : "struct");

  if (!pParentRTTI)
    ezLog::Info("Type has no parent class.");
  else
  {
    PrintStats(pParentRTTI);
  }
}

int main()
{
  ezStartup::StartupCore();

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  PrintStats(TestBase::GetStaticRTTI());
  PrintStats(TestA::GetStaticRTTI());
  PrintStats(TestAB::GetStaticRTTI());
  PrintStats(TestB::GetStaticRTTI());
  PrintStats(StructA::GetStaticRTTI());

  {
    EZ_LOG_BLOCK("Derived From TestBase");

    ezLog::Info("TestBase: %s", TestBase::GetStaticRTTI()->IsDerivedFrom<TestBase>() ? "yes" : "no");
    ezLog::Info("TestA: %s", TestA::GetStaticRTTI()->IsDerivedFrom<TestBase>() ? "yes" : "no");
    ezLog::Info("TestAB: %s", TestAB::GetStaticRTTI()->IsDerivedFrom<TestBase>() ? "yes" : "no");
    ezLog::Info("TestB: %s", TestB::GetStaticRTTI()->IsDerivedFrom<TestBase>() ? "yes" : "no");
    ezLog::Info("StructA: %s", StructA::GetStaticRTTI()->IsDerivedFrom<TestBase>() ? "yes" : "no");
  }

  {
    EZ_LOG_BLOCK("Derived From TestA");

    ezLog::Info("TestBase: %s", TestBase::GetStaticRTTI()->IsDerivedFrom<TestA>() ? "yes" : "no");
    ezLog::Info("TestA: %s", TestA::GetStaticRTTI()->IsDerivedFrom<TestA>() ? "yes" : "no");
    ezLog::Info("TestAB: %s", TestAB::GetStaticRTTI()->IsDerivedFrom<TestA>() ? "yes" : "no");
    ezLog::Info("TestB: %s", TestB::GetStaticRTTI()->IsDerivedFrom<TestA>() ? "yes" : "no");
    ezLog::Info("StructA: %s", StructA::GetStaticRTTI()->IsDerivedFrom<TestA>() ? "yes" : "no");
  }

  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  ezStartup::ShutdownBase();
}