#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>

#include "Main.h"

EZ_IMPLEMENT_MESSAGE_TYPE(ezTestMessage);
EZ_IMPLEMENT_MESSAGE_TYPE(ezTestMessage2);

EZ_BEGIN_REFLECTED_TYPE(TestBase, ezReflectedClass, ezRTTIDefaultAllocator<TestBase>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("TestProp1", m_Test),
    EZ_MEMBER_PROPERTY("TestProp2", m_Test2),
    EZ_ACCESSOR_PROPERTY("Float1", GetFloat1, SetFloat1),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("Float1 (ro)", GetFloat1)
  EZ_END_PROPERTIES
EZ_END_REFLECTED_TYPE(TestBase);

EZ_BEGIN_REFLECTED_TYPE(MyStruct, ezNoBase, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Int", i2),
    EZ_MEMBER_PROPERTY("Float", f1),
  EZ_END_PROPERTIES
EZ_END_REFLECTED_TYPE(MyStruct);

EZ_BEGIN_REFLECTED_TYPE(TestClassA, TestBase, ezRTTIDefaultAllocator<TestClassA>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("MyStruct", m_MyStruct),
    EZ_MEMBER_PROPERTY_READ_ONLY("MyStruct (ro)", m_MyStruct),
    EZ_FUNCTION_PROPERTY("Be Useful", DoSomething)
  EZ_END_PROPERTIES
  EZ_BEGIN_MESSAGEHANDLERS
    EZ_MESSAGE_HANDLER(ezTestMessage, HandleMessage),
    EZ_MESSAGE_HANDLER(const ezTestMessage2, HandleMessage2) // I have no idea how one could get a function pointer to the right version of an overloaded function
  EZ_END_MESSAGEHANDLERS
EZ_END_REFLECTED_TYPE(TestClassA);

void PrintTypeInfo(const ezRTTI* pRTTI, bool bAllocate = false, bool bRecursive = true, void* pInstance = NULL)
{
  if (pRTTI == NULL)
    return;

  ezLog::Info("Type: '%s' (%i Bytes, Plugin: '%s')", pRTTI->GetTypeName(), pRTTI->GetTypeSize(), pRTTI->GetPluginName());

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

  if (pRTTI->GetMessageHandlers().GetCount() > 0)
  {
    ezLog::Dev("  Type has Message Handlers:");

    for (ezUInt32 m = 0; m < pRTTI->GetMessageHandlers().GetCount(); ++m)
    {
      ezLog::Dev("    MessageID: %i", pRTTI->GetMessageHandlers()[m]->GetMessageTypeID());
    }

    if (pInstance)
    {
      ezMessage msg;

      for (ezUInt32 m = 0; m < pRTTI->GetMessageHandlers().GetCount(); ++m)
      {
        pRTTI->GetMessageHandlers()[m]->HandleMessage(pInstance, &msg);
      }
    }
  }

  {
    for (ezUInt32 p = 0; p < pRTTI->GetProperties().GetCount(); ++p)
    {
      ezLog::Info("  Property: '%s'", pRTTI->GetProperties()[p]->GetPropertyName());

      if (pInstance != NULL && pRTTI->GetProperties()[p]->GetCategory() == ezAbstractProperty::Function)
      {
        ezAbstractFunctionProperty* pFunc = (ezAbstractFunctionProperty*) pRTTI->GetProperties()[p];
        pFunc->Execute(pInstance);
      }

      if (pInstance != NULL && pRTTI->GetProperties()[p]->GetCategory() == ezAbstractProperty::Member)
      {
        ezAbstractMemberProperty* pMember = (ezAbstractMemberProperty*) pRTTI->GetProperties()[p];

        if (pMember->IsReadOnly())
          ezLog::Debug("    (read-only)");

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
        else
        if (pMember->GetPropertyPointer(pInstance) != NULL)
        {
          // only if we do not know the specific type, we try to drill down further

          EZ_LOG_BLOCK("Property Struct");
          PrintTypeInfo(pPropRTTI, false, true, pMember->GetPropertyPointer(pInstance));
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

    ezTestMessage tm;
    a.GetDynamicRTTI()->HandleMessageOfType(&a, &tm);

    ezTestMessage2 tm2;
    a.GetDynamicRTTI()->HandleMessageOfType(&a, &tm2);
  }

  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  ezStartup::ShutdownBase();
}