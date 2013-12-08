#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>

#include "Test.h"

EZ_ADD_MEMBER_PROPERTY(ezVec3, x);
EZ_ADD_MEMBER_PROPERTY(ezVec3, y);
EZ_ADD_MEMBER_PROPERTY(ezVec3, z);

EZ_ADD_PROPERTY_WITH_ACCESSOR(TestBase, SomeData);
EZ_ADD_MEMBER_PROPERTY(TestBase, m_Vector);
EZ_ADD_MEMBER_PROPERTY(TestBase, m_ReflectedMember);

EZ_ADD_MEMBER_PROPERTY(TestA, m_TestBaseMember);

EZ_ADD_PROPERTY_WITH_ACCESSOR(TestAB, SomeBool);
EZ_ADD_MEMBER_PROPERTY(TestAB, privateint);
EZ_ADD_MEMBER_PROPERTY(TestAB, privateint2);

EZ_ADD_MEMBER_PROPERTY(stuff::StructB, StructB_Float);
EZ_ADD_MEMBER_PROPERTY(stuff::StructB, StructB_Bool);
EZ_ADD_ARRAY_PROPERTY(stuff::StructB, m_Deque, float);

EZ_ADD_PROPERTY_WITH_ACCESSOR(StructA, SomeFloat);
EZ_ADD_MEMBER_PROPERTY(StructA, m_SubProperty);
EZ_ADD_PROPERTY_WITH_ACCESSOR(StructA, m_Vec3);
EZ_ADD_MEMBER_PROPERTY(StructA, m_Vec3);

//ezCallAtStartup bla([] { printf("Look Ma! A Lamba!"); } );

class FuncTestClass
{
public:
  void MyMemberFunc(int i, ezVec3 v)
  {
    ezLog::Success("%i: Called MyMemberFunc with %i and %.2f | %.2f | %.2f", m_iInstance, i, v.x, v.y, v.z);
  }

  int m_iInstance;
};

void PrintStats(const ezTypeRTTI* pRTTI)
{
  const ezTypeRTTI* pParentRTTI = pRTTI->GetParentRTTI();

  EZ_LOG_BLOCK("Reflected Class", pRTTI->GetTypeName());

  ezLog::Info("Name: %s", pRTTI->GetTypeName());
  ezLog::Info("ID: %i", pRTTI->GetTypeID());

  {
    EZ_LOG_BLOCK("Properties");
    
    for (size_t i = 0; i < pRTTI->m_Properties.size(); ++i)
    {
      ezLog::Info("%s : %s", pRTTI->m_Properties[i]->GetPropertyName(), pRTTI->m_Properties[i]->GetPropertyType()->GetTypeName());
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
  EZ_LOG_BLOCK("Property Values", pRTTI->GetTypeName());

  for (size_t i = 0; i < pRTTI->m_Properties.size(); ++i)
  {
    ezStringBuilder val;

    const ezInt32 iTypeID = pRTTI->m_Properties[i]->GetPropertyType()->GetTypeID();

    if (iTypeID == GetStaticRTTI<stuff::StructB>()->GetTypeID()) // works with namespaces, as long as we don't use the actual string for type inference
    {
      ezLog::Info("Property is of type 'stuff::StructB'");
    }

    const auto& SubProperties = pRTTI->m_Properties[i]->GetPropertyType()->m_Properties;

    if (!SubProperties.empty())
    {
      void* pProperty = pRTTI->m_Properties[i]->GetPropertyPointer(pReflected);

      ezLog::Info("Sub Property: %s", pRTTI->m_Properties[i]->GetPropertyName());
      if (pProperty)
        PrintPropertyValues(pRTTI->m_Properties[i]->GetPropertyType(), pProperty);
      else
        ezLog::Info("%s %s : uses accessors -> no recursion possible", pRTTI->m_Properties[i]->GetPropertyType()->GetTypeName(), pRTTI->m_Properties[i]->GetPropertyName());

      continue;
    }

    if (iTypeID == GetStaticRTTI<bool>()->GetTypeID())
    {
      val = ((ezTypedProperty<bool>*) pRTTI->m_Properties[i])->GetValue(pReflected) ? "true" : "false";
    }
    else if (iTypeID == GetStaticRTTI<ezInt32>()->GetTypeID())
    {
      val.Format("%i", ((ezTypedProperty<int>*) pRTTI->m_Properties[i])->GetValue(pReflected));
    }
    else if (iTypeID == GetStaticRTTI<float>()->GetTypeID())
    {
      val.Format("%.2f", ((ezTypedProperty<float>*) pRTTI->m_Properties[i])->GetValue(pReflected));
    }
    else if (iTypeID == GetStaticRTTI<ezArrayProperty>()->GetTypeID())
    {
      val.Format("array");

      ezArrayProperty* pArray = (ezArrayProperty*) pRTTI->m_Properties[i];
      void* pSubReflected = pRTTI->m_Properties[i]->GetPropertyPointer(pReflected);

      const ezTypeRTTI* pSubType = pArray->GetElementType();

      EZ_LOG_BLOCK("Array Property", pSubType->GetTypeName());

      ezLog::Info("Elements: %i", pArray->GetCount(pSubReflected));

      for (ezUInt32 el = 0; el < pArray->GetCount(pSubReflected); ++el)
      {
        if (pSubType->GetTypeID() == GetStaticRTTI<float>()->GetTypeID())
        {
          float* pData = (float*) pArray->GetElement(pSubReflected, el);
          ezLog::Info("[%i] = %.2f", el, *pData);
        }        
      }

    }
    else
    {
      val.Format("unknown");
    }

    ezLog::Info("%s %s = %s", pRTTI->m_Properties[i]->GetPropertyType()->GetTypeName(), pRTTI->m_Properties[i]->GetPropertyName(), val.GetData());
  }

  if (pRTTI->GetParentRTTI())
    PrintPropertyValues(pRTTI->GetParentRTTI(), pReflected);
}

int main()
{
  ezStartup::StartupCore();

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  PrintStats(GetStaticRTTI<TestBase>());
  PrintStats(GetStaticRTTI<TestA>());
  PrintStats(GetStaticRTTI<TestAB>());
  PrintStats(GetStaticRTTI<TestB>());
  PrintStats(GetStaticRTTI<StructA>());

  {
    EZ_LOG_BLOCK("Derived From TestBase");

    ezLog::Info("TestBase: %s", GetStaticRTTI<TestBase>()->IsDerivedFrom<TestBase>() ? "yes" : "no");
    ezLog::Info("TestA: %s",    GetStaticRTTI<TestA>()->IsDerivedFrom<TestBase>() ? "yes" : "no");
    ezLog::Info("TestAB: %s",   GetStaticRTTI<TestAB>()->IsDerivedFrom<TestBase>() ? "yes" : "no");
    ezLog::Info("TestB: %s",    GetStaticRTTI<TestB>()->IsDerivedFrom<TestBase>() ? "yes" : "no");
    ezLog::Info("StructA: %s",  GetStaticRTTI<StructA>()->IsDerivedFrom<TestBase>() ? "yes" : "no");
  }

  {
    EZ_LOG_BLOCK("Derived From TestA");

    ezLog::Info("TestBase: %s", GetStaticRTTI<TestBase>()->IsDerivedFrom<TestA>() ? "yes" : "no");
    ezLog::Info("TestA: %s",    GetStaticRTTI<TestA>()->IsDerivedFrom<TestA>() ? "yes" : "no");
    ezLog::Info("TestAB: %s",   GetStaticRTTI<TestAB>()->IsDerivedFrom<TestA>() ? "yes" : "no");
    ezLog::Info("TestB: %s",    GetStaticRTTI<TestB>()->IsDerivedFrom<TestA>() ? "yes" : "no");
    ezLog::Info("StructA: %s",  GetStaticRTTI<StructA>()->IsDerivedFrom<TestA>() ? "yes" : "no");
  }

  {
    EZ_LOG_BLOCK("IsOfType TestBase");

    ezLog::Info("TestBase: %s", GetStaticRTTI<TestBase>()->IsOfType<TestBase>() ? "yes" : "no");
    ezLog::Info("TestA: %s",    GetStaticRTTI<TestA>()->IsOfType<TestBase>() ? "yes" : "no");
    ezLog::Info("TestAB: %s",   GetStaticRTTI<TestAB>()->IsOfType<TestBase>() ? "yes" : "no");
    ezLog::Info("TestB: %s",    GetStaticRTTI<TestB>()->IsOfType<TestBase>() ? "yes" : "no");
    ezLog::Info("StructA: %s",  GetStaticRTTI<StructA>()->IsOfType<TestBase>() ? "yes" : "no");
  }

  {
    TestAB blub;
    PrintPropertyValues(GetStaticRTTI<TestAB>(), &blub);
  }

  {
    StructA blub;
    PrintPropertyValues(GetStaticRTTI<StructA>(), &blub);
  }

  {
    stuff::StructB blub;
    blub.m_Deque.PushBack(2.3f);
    blub.m_Deque.PushBack(4.5f);
    PrintPropertyValues(GetStaticRTTI<stuff::StructB>(), &blub);
  }

  // Functions
  {
    ezReflectedFunctionImpl<FuncTestClass, int, ezVec3> FuncWrap(&FuncTestClass::MyMemberFunc);

    ezVariant Vars[5];
    Vars[0] = 5;
    Vars[1] = ezVec3(1, 2, 3);

    ezArrayPtr<ezVariant> Params(Vars, FuncWrap.GetParameterCount());

    FuncTestClass t1;
    t1.m_iInstance = 17;

    FuncWrap.Execute(&t1, Params);
  }

  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  ezStartup::ShutdownBase();
}