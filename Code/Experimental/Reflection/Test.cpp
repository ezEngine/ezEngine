#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>

#include "Test.h"

EZ_IMPLEMENT_REFLECTED_CLASS(TestBase);
EZ_IMPLEMENT_REFLECTED_CLASS(TestA);
EZ_IMPLEMENT_REFLECTED_CLASS(TestAB);
EZ_IMPLEMENT_REFLECTED_CLASS(TestB);

EZ_IMPLEMENT_REFLECTED_STRUCT(StructA);

EZ_ADD_PROPERTY(TestBase, int, SomeData);

EZ_ADD_PROPERTY(TestAB, bool, SomeBool);
EZ_ADD_PROPERTY_NO_ACCESSOR(TestAB, int, privateint);
EZ_ADD_PROPERTY_NO_ACCESSOR(TestAB, int, privateint2);

EZ_ADD_PROPERTY(StructA, float, SomeFloat);

ezCallAtStartup bla([] { printf("Look Ma! A Lamba!"); } );

const char* GetPropertyTypeString(ezPropertyType type, ezAbstractProperty* pProperty)
{
  switch (type)
  {
  case Bool:    return "bool";
  case Int32:   return "int32";
  case Float:   return "float";
  }

  return "Unknown Type";
}

void PrintStats(const ezTypeRTTI* pRTTI)
{
  const ezTypeRTTI* pParentRTTI = pRTTI->GetParentRTTI();

  EZ_LOG_BLOCK("Reflected Class", pRTTI->GetTypeName());

  ezLog::Info("Name: %s", pRTTI->GetTypeName());
  ezLog::Info("ID: %i", pRTTI->GetTypeID());
  ezLog::Info("Type: %s", pRTTI->GetTypeMode() == ezTypeRTTI::Class ? "class" : "struct");

  {
    EZ_LOG_BLOCK("Properties");
    
    for (size_t i = 0; i < pRTTI->m_Properties.size(); ++i)
    {
      ezLog::Info("%s : %s", pRTTI->m_Properties[i]->GetPropertyName(), GetPropertyTypeString(pRTTI->m_Properties[i]->GetPropertyType(), pRTTI->m_Properties[i]));
    }
  }

  if (!pParentRTTI)
    ezLog::Info("Type has no parent class.");
  else
  {
    PrintStats(pParentRTTI);
  }
}

void PrintPropertyValues(const ezTypeRTTI* pRTTI, const void* pReflected)
{
  EZ_LOG_BLOCK("Properties", pRTTI->GetTypeName());

  for (size_t i = 0; i < pRTTI->m_Properties.size(); ++i)
  {
    ezStringBuilder val;

    switch (pRTTI->m_Properties[i]->GetPropertyType())
    {
    case Bool:
      val = ((ezTypedProperty<bool>*) pRTTI->m_Properties[i])->GetValue(pReflected) ? "true" : "false";
      break;
    case Int32:
      val.Format("%i", ((ezTypedProperty<int>*) pRTTI->m_Properties[i])->GetValue(pReflected));
      break;
    case Float:
      val.Format("%.2f", ((ezTypedProperty<float>*) pRTTI->m_Properties[i])->GetValue(pReflected));
      break;
    }

    ezLog::Info("%s %s = %s", GetPropertyTypeString(pRTTI->m_Properties[i]->GetPropertyType(), pRTTI->m_Properties[i]), pRTTI->m_Properties[i]->GetPropertyName(), val.GetData());
  }

  if (pRTTI->GetParentRTTI())
    PrintPropertyValues(pRTTI->GetParentRTTI(), pReflected);
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

  {
    EZ_LOG_BLOCK("IsOfType TestBase");

    ezLog::Info("TestBase: %s", TestBase::GetStaticRTTI()->IsOfType<TestBase>() ? "yes" : "no");
    ezLog::Info("TestA: %s", TestA::GetStaticRTTI()->IsOfType<TestBase>() ? "yes" : "no");
    ezLog::Info("TestAB: %s", TestAB::GetStaticRTTI()->IsOfType<TestBase>() ? "yes" : "no");
    ezLog::Info("TestB: %s", TestB::GetStaticRTTI()->IsOfType<TestBase>() ? "yes" : "no");
    ezLog::Info("StructA: %s", StructA::GetStaticRTTI()->IsOfType<TestBase>() ? "yes" : "no");
  }

  {
    TestAB blub;
    PrintPropertyValues(TestAB::GetStaticRTTI(), &blub);
  }

  {
    StructA blub;
    PrintPropertyValues(StructA::GetStaticRTTI(), &blub);
  }

  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  ezStartup::ShutdownBase();
}