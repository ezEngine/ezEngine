#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>

#include "Main.h"

EZ_BEGIN_REFLECTED_TYPE(TestBase, ezReflectedClass, ezRTTIDefaultAllocator<TestBase>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("TestProp1", m_Test),
    EZ_MEMBER_PROPERTY("TestProp2", m_Test2),
    EZ_ACCESSOR_PROPERTY("Float1", GetFloat1, SetFloat1)
  EZ_END_PROPERTIES
EZ_END_REFLECTED_TYPE(TestBase);

EZ_BEGIN_REFLECTED_TYPE(TestClassA, TestBase, ezRTTIDefaultAllocator<TestClassA>);
  EZ_BEGIN_PROPERTIES
    EZ_FUNCTION_PROPERTY("Be Useful", DoSomething)
  EZ_END_PROPERTIES
EZ_END_REFLECTED_TYPE(TestClassA);

void PrintTypeInfo(const ezRTTI* pRTTI, bool bAllocate = false, bool bRecursive = true, void* pInstance = NULL)
{
  if (pRTTI == NULL)
    return;

  ezLog::Info("Type: '%s' (%i Bytes)", pRTTI->GetTypeName(), pRTTI->GetTypeSize());

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
    {
      ezLog::Info("  Property: '%s'", pRTTI->GetProperty(p)->GetPropertyName());

      if (pInstance != NULL && pRTTI->GetProperty(p)->GetCategory() == ezAbstractProperty::Function)
      {
        ezAbstractFunctionProperty* pFunc = (ezAbstractFunctionProperty*) pRTTI->GetProperty(p);
        pFunc->Execute(pInstance);
      }

      if (pInstance != NULL && pRTTI->GetProperty(p)->GetCategory() == ezAbstractProperty::Member)
      {
        ezAbstractMemberProperty* pMember = (ezAbstractMemberProperty*) pRTTI->GetProperty(p);

        const ezRTTI* pPropRTTI = pMember->GetPropertyType();

        if (pPropRTTI == ezGetStaticRTTI<float>())
        {
          ezTypedMemberProperty<float>* pFinal = (ezTypedMemberProperty<float>*) pMember;

          ezLog::Dev("    Value: %.2f", pFinal->GetValue(pInstance));
        }
        else
        if (pPropRTTI == ezGetStaticRTTI<int>())
        {
          ezTypedMemberProperty<int>* pFinal = (ezTypedMemberProperty<int>*) pMember;

          ezLog::Dev("    Value: %i", pFinal->GetValue(pInstance));
        }
      }
    }
  }

  if (!bRecursive)
    return;

  if (pRTTI->GetParentType() != NULL)
  {
    EZ_LOG_BLOCK("Parent Type");
    PrintTypeInfo(pRTTI->GetParentType(), false, true, pInstance);
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

  {
    EZ_LOG_BLOCK("Instance Test");

    TestClassA a;
    PrintTypeInfo(a.GetDynamicRTTI(), false, true, &a);

  }

  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  ezStartup::ShutdownBase();
}