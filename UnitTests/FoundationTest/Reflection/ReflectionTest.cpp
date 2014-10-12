#include <PCH.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>


EZ_CREATE_SIMPLE_TEST_GROUP(Reflection);

/// \todo Test Array Properties (once they are implemented)
/// \todo Test Enum Property Type (when implemented)

struct ezTestStruct
{
  EZ_ALLOW_PRIVATE_PROPERTIES(ezTestStruct);

public:
  ezTestStruct()
  {
    m_fFloat1 = 1.1f;
    m_iInt2 = 2;
    m_vProperty3.Set(3, 4, 5);
    m_UInt8 = 6;
  }

  float m_fFloat1;
  ezUInt8 m_UInt8;

private:
  void SetInt(ezInt32 i) { m_iInt2 = i; }
  ezInt32 GetInt() const { return m_iInt2; }

  ezInt32 m_iInt2;
  ezVec3 m_vProperty3;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTestStruct);

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezTestStruct, ezNoBase, 7, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Float", m_fFloat1),
    EZ_MEMBER_PROPERTY_READ_ONLY("Vector", m_vProperty3),
    EZ_ACCESSOR_PROPERTY("Int", GetInt, SetInt),
    EZ_MEMBER_PROPERTY("UInt8", m_UInt8),
  EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();

struct ezTestStruct3
{
  EZ_ALLOW_PRIVATE_PROPERTIES(ezTestStruct3);

public:
  ezTestStruct3()
  {
    m_fFloat1 = 1.1f;
    m_iInt2 = 2;
    m_UInt8 = 6;
  }

  double m_fFloat1;
  ezInt16 m_UInt8;

private:
  void SetInt(ezUInt32 i) { m_iInt2 = i; }
  ezUInt32 GetInt() const { return m_iInt2; }

  ezInt32 m_iInt2;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTestStruct3);

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezTestStruct3, ezNoBase, 71, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Float", m_fFloat1),
    EZ_ACCESSOR_PROPERTY("Int", GetInt, SetInt),
    EZ_MEMBER_PROPERTY("UInt8", m_UInt8),
  EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();


class ezTestClass1 : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestClass1);

public:
  ezTestClass1()
  {
    m_MyVector.Set(3, 4, 5);

    m_Struct.m_fFloat1 = 33.3f;
  }

  ezVec3 GetVector() const { return m_MyVector; }

  ezTestStruct m_Struct;
  ezVec3 m_MyVector;
  ezColor m_Color;
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestClass1, ezReflectedClass, 11, ezRTTIDefaultAllocator<ezTestClass1>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Sub Struct", m_Struct),
    EZ_MEMBER_PROPERTY("Color", m_Color),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("Sub Vector", GetVector)
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

class ezTestClass2 : public ezTestClass1
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestClass2);

public:
  ezTestClass2()
  {
    m_Text = "Legen";
  }

  const char* GetText() const { return m_Text.GetData(); }
  void SetText(const char* sz) { m_Text = sz; }

  ezTime m_Time;

private:
  ezString m_Text;
};

struct ezTestClass2Allocator : public ezRTTIAllocator
{
  virtual void* Allocate() override
  {
    ++m_iAllocs;

    return EZ_DEFAULT_NEW(ezTestClass2);
  }

  virtual void Deallocate(void* pObject) override
  {
    ++m_iDeallocs;

    ezTestClass2* pPointer = (ezTestClass2*) pObject;
    EZ_DEFAULT_DELETE(pPointer);
  }

  static ezInt32 m_iAllocs;
  static ezInt32 m_iDeallocs;
};

ezInt32 ezTestClass2Allocator::m_iAllocs = 0;
ezInt32 ezTestClass2Allocator::m_iDeallocs = 0;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestClass2, ezTestClass1, 22, ezTestClass2Allocator);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Text", GetText, SetText),
    EZ_MEMBER_PROPERTY("Time", m_Time)
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

class ezTestClass2b : ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestClass2b);

public:
  ezTestClass2b()
  {
    m_Text = "Tut";
  }

  const char* GetText() const { return m_Text.GetData(); }
  void SetText(const char* sz) { m_Text = sz; }

  ezTestStruct3 m_Struct;
  ezColor m_Color;

private:
  ezString m_Text;
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestClass2b, ezReflectedClass, 24, ezRTTIDefaultAllocator<ezTestClass2b>);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Text2b", GetText, SetText),
    EZ_MEMBER_PROPERTY("Sub Struct", m_Struct),
    EZ_MEMBER_PROPERTY("Color", m_Color),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_CREATE_SIMPLE_TEST(Reflection, Types)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Iterate All")
  {
    bool bFoundStruct = false;
    bool bFoundClass1 = false;
    bool bFoundClass2 = false;

    ezRTTI* pRtti = ezRTTI::GetFirstInstance();

    while (pRtti)
    {
      if (ezStringUtils::IsEqual(pRtti->GetTypeName(), "ezTestStruct"))
        bFoundStruct = true;
      if (ezStringUtils::IsEqual(pRtti->GetTypeName(), "ezTestClass1"))
        bFoundClass1 = true;
      if (ezStringUtils::IsEqual(pRtti->GetTypeName(), "ezTestClass2"))
        bFoundClass2 = true;

      EZ_TEST_STRING(pRtti->GetPluginName(), "Static");

      pRtti = pRtti->GetNextInstance();
    }

    EZ_TEST_BOOL(bFoundStruct);
    EZ_TEST_BOOL(bFoundClass1);
    EZ_TEST_BOOL(bFoundClass2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindTypeByName")
  {
    ezRTTI* pFloat = ezRTTI::FindTypeByName("float");
    EZ_TEST_BOOL(pFloat != nullptr);
    EZ_TEST_STRING(pFloat->GetTypeName(), "float");

    ezRTTI* pStruct = ezRTTI::FindTypeByName("ezTestStruct");
    EZ_TEST_BOOL(pStruct != nullptr);
    EZ_TEST_STRING(pStruct->GetTypeName(), "ezTestStruct");

    ezRTTI* pClass2 = ezRTTI::FindTypeByName("ezTestClass2");
    EZ_TEST_BOOL(pClass2 != nullptr);
    EZ_TEST_STRING(pClass2->GetTypeName(), "ezTestClass2");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetProperties")
  {
    {
      ezRTTI* pType = ezRTTI::FindTypeByName("ezTestStruct");

      auto Props = pType->GetProperties();
      EZ_TEST_INT(Props.GetCount(), 4);
      EZ_TEST_STRING(Props[0]->GetPropertyName(), "Float");
      EZ_TEST_STRING(Props[1]->GetPropertyName(), "Vector");
      EZ_TEST_STRING(Props[2]->GetPropertyName(), "Int");
      EZ_TEST_STRING(Props[3]->GetPropertyName(), "UInt8");
    }

    {
      ezRTTI* pType = ezRTTI::FindTypeByName("ezTestClass2");

      auto Props = pType->GetProperties();
      EZ_TEST_INT(Props.GetCount(), 2);
      EZ_TEST_STRING(Props[0]->GetPropertyName(), "Text");
      EZ_TEST_STRING(Props[1]->GetPropertyName(), "Time");

      ezHybridArray<ezAbstractProperty*, 32> AllProps;
      pType->GetAllProperties(AllProps);

      EZ_TEST_INT(AllProps.GetCount(), 5);
      EZ_TEST_STRING(AllProps[0]->GetPropertyName(), "Sub Struct");
      EZ_TEST_STRING(AllProps[1]->GetPropertyName(), "Color");
      EZ_TEST_STRING(AllProps[2]->GetPropertyName(), "Sub Vector");
      EZ_TEST_STRING(AllProps[3]->GetPropertyName(), "Text");
      EZ_TEST_STRING(AllProps[4]->GetPropertyName(), "Time");
    }
  }

#if EZ_ENABLED(EZ_SUPPORTS_DYNAMIC_PLUGINS)

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Types From Plugin")
  {
    EZ_TEST_BOOL(ezPlugin::LoadPlugin("ezFoundationTest_Plugin1") == EZ_SUCCESS);

    ezRTTI* pStruct2 = ezRTTI::FindTypeByName("ezTestStruct2");
    EZ_TEST_BOOL(pStruct2 != nullptr);
    EZ_TEST_STRING(pStruct2->GetTypeName(), "ezTestStruct2");


    bool bFoundStruct2 = false;

    ezRTTI* pRtti = ezRTTI::GetFirstInstance();

    while (pRtti)
    {
      if (ezStringUtils::IsEqual(pRtti->GetTypeName(), "ezTestStruct2"))
      {
        bFoundStruct2 = true;

        EZ_TEST_STRING(pRtti->GetPluginName(), "ezFoundationTest_Plugin1");

        void* pInstance = pRtti->GetAllocator()->Allocate();
        EZ_TEST_BOOL(pInstance != nullptr);

        ezAbstractProperty* pProp = pRtti->FindPropertyByName("Float 2");

        EZ_TEST_BOOL(pProp != nullptr);

        EZ_TEST_BOOL(pProp->GetCategory() == ezAbstractProperty::Member);
        ezAbstractMemberProperty* pAbsMember = (ezAbstractMemberProperty*) pProp;

        EZ_TEST_BOOL(pAbsMember->GetPropertyType() == ezGetStaticRTTI<float>());

        ezTypedMemberProperty<float>* pMember = (ezTypedMemberProperty<float>*) pAbsMember;

        EZ_TEST_FLOAT(pMember->GetValue(pInstance), 42.0f, 0);
        pMember->SetValue(pInstance, 43.0f);
        EZ_TEST_FLOAT(pMember->GetValue(pInstance), 43.0f, 0);

        pRtti->GetAllocator()->Deallocate(pInstance);
      }
      else
      {
        EZ_TEST_STRING(pRtti->GetPluginName(), "Static");
      }

      pRtti = pRtti->GetNextInstance();
    }

    EZ_TEST_BOOL(bFoundStruct2);

    EZ_TEST_BOOL(ezPlugin::UnloadPlugin("ezFoundationTest_Plugin1") == EZ_SUCCESS);
  }
#endif
}

EZ_CREATE_SIMPLE_TEST(Reflection, Hierarchies)
{
  ezTestClass2Allocator::m_iAllocs = 0;
  ezTestClass2Allocator::m_iDeallocs = 0;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezTestStruct")
  {
    const ezRTTI* pRtti = ezGetStaticRTTI<ezTestStruct>();

    EZ_TEST_STRING(pRtti->GetTypeName(), "ezTestStruct");
    EZ_TEST_INT(pRtti->GetTypeSize(), sizeof(ezTestStruct));
    EZ_TEST_BOOL(pRtti->GetVariantType() == ezVariant::Type::Invalid);

    EZ_TEST_BOOL(pRtti->GetParentType() == nullptr);

    EZ_TEST_BOOL(!pRtti->GetAllocator()->CanAllocate());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezTestClass1")
  {
    const ezRTTI* pRtti = ezGetStaticRTTI<ezTestClass1>();

    EZ_TEST_STRING(pRtti->GetTypeName(), "ezTestClass1");
    EZ_TEST_INT(pRtti->GetTypeSize(), sizeof(ezTestClass1));
    EZ_TEST_BOOL(pRtti->GetVariantType() == ezVariant::Type::ReflectedPointer);

    EZ_TEST_BOOL(pRtti->GetParentType() == ezGetStaticRTTI<ezReflectedClass>());

    EZ_TEST_BOOL(pRtti->GetAllocator()->CanAllocate());

    ezTestClass1* pInstance = (ezTestClass1*) pRtti->GetAllocator()->Allocate();
    EZ_TEST_BOOL(pInstance != nullptr);

    EZ_TEST_BOOL(pInstance->GetDynamicRTTI() == ezGetStaticRTTI<ezTestClass1>());
    pInstance->GetDynamicRTTI()->GetAllocator()->Deallocate(pInstance);

    EZ_TEST_BOOL(pRtti->IsDerivedFrom<ezReflectedClass>());
    EZ_TEST_BOOL(pRtti->IsDerivedFrom(ezGetStaticRTTI<ezReflectedClass>()));

    EZ_TEST_BOOL(pRtti->IsDerivedFrom<ezTestClass1>());
    EZ_TEST_BOOL(pRtti->IsDerivedFrom(ezGetStaticRTTI<ezTestClass1>()));

    EZ_TEST_BOOL(!pRtti->IsDerivedFrom<ezVec3>());
    EZ_TEST_BOOL(!pRtti->IsDerivedFrom(ezGetStaticRTTI<ezVec3>()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezTestClass2")
  {
    const ezRTTI* pRtti = ezGetStaticRTTI<ezTestClass2>();

    EZ_TEST_STRING(pRtti->GetTypeName(), "ezTestClass2");
    EZ_TEST_INT(pRtti->GetTypeSize(), sizeof(ezTestClass2));
    EZ_TEST_BOOL(pRtti->GetVariantType() == ezVariant::Type::ReflectedPointer);

    EZ_TEST_BOOL(pRtti->GetParentType() == ezGetStaticRTTI<ezTestClass1>());

    EZ_TEST_BOOL(pRtti->GetAllocator()->CanAllocate());

    EZ_TEST_INT(ezTestClass2Allocator::m_iAllocs, 0);
    EZ_TEST_INT(ezTestClass2Allocator::m_iDeallocs, 0);

    ezTestClass2* pInstance = (ezTestClass2*) pRtti->GetAllocator()->Allocate();
    EZ_TEST_BOOL(pInstance != nullptr);

    EZ_TEST_BOOL(pInstance->GetDynamicRTTI() == ezGetStaticRTTI<ezTestClass2>());

    EZ_TEST_INT(ezTestClass2Allocator::m_iAllocs, 1);
    EZ_TEST_INT(ezTestClass2Allocator::m_iDeallocs, 0);

    pInstance->GetDynamicRTTI()->GetAllocator()->Deallocate(pInstance);

    EZ_TEST_INT(ezTestClass2Allocator::m_iAllocs, 1);
    EZ_TEST_INT(ezTestClass2Allocator::m_iDeallocs, 1);

    EZ_TEST_BOOL(pRtti->IsDerivedFrom<ezTestClass1>());
    EZ_TEST_BOOL(pRtti->IsDerivedFrom(ezGetStaticRTTI<ezTestClass1>()));

    EZ_TEST_BOOL(pRtti->IsDerivedFrom<ezTestClass2>());
    EZ_TEST_BOOL(pRtti->IsDerivedFrom(ezGetStaticRTTI<ezTestClass2>()));

    EZ_TEST_BOOL(pRtti->IsDerivedFrom<ezReflectedClass>());
    EZ_TEST_BOOL(pRtti->IsDerivedFrom(ezGetStaticRTTI<ezReflectedClass>()));

    EZ_TEST_BOOL(!pRtti->IsDerivedFrom<ezVec3>());
    EZ_TEST_BOOL(!pRtti->IsDerivedFrom(ezGetStaticRTTI<ezVec3>()));
  }

}

EZ_CREATE_SIMPLE_TEST(Reflection, MemberProperties)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezTestStruct")
  {
    const ezRTTI* pRtti = ezGetStaticRTTI<ezTestStruct>();

    {
      ezAbstractProperty* pProp = pRtti->FindPropertyByName("Float");
      EZ_TEST_BOOL(pProp != nullptr);

      EZ_TEST_BOOL(pProp->GetCategory() == ezAbstractProperty::Member);
      ezAbstractMemberProperty* pAbs = (ezAbstractMemberProperty*) pProp;

      EZ_TEST_BOOL(pAbs->GetPropertyType() == ezGetStaticRTTI<float>());
      ezTypedMemberProperty<float>* pMember = (ezTypedMemberProperty<float>*) pAbs;

      EZ_TEST_BOOL(!pMember->IsReadOnly());
    }

    {
      ezAbstractProperty* pProp = pRtti->FindPropertyByName("Int");
      EZ_TEST_BOOL(pProp != nullptr);

      EZ_TEST_BOOL(pProp->GetCategory() == ezAbstractProperty::Member);
      ezAbstractMemberProperty* pAbs = (ezAbstractMemberProperty*) pProp;

      EZ_TEST_BOOL(pAbs->GetPropertyType() == ezGetStaticRTTI<int>());
      ezTypedMemberProperty<int>* pMember = (ezTypedMemberProperty<int>*) pAbs;

      EZ_TEST_BOOL(!pMember->IsReadOnly());
    }

    {
      ezAbstractProperty* pProp = pRtti->FindPropertyByName("Vector");
      EZ_TEST_BOOL(pProp != nullptr);

      EZ_TEST_BOOL(pProp->GetCategory() == ezAbstractProperty::Member);
      ezAbstractMemberProperty* pAbs = (ezAbstractMemberProperty*) pProp;

      EZ_TEST_BOOL(pAbs->GetPropertyType() == ezGetStaticRTTI<ezVec3>());
      ezTypedMemberProperty<ezVec3>* pMember = (ezTypedMemberProperty<ezVec3>*) pAbs;

      EZ_TEST_BOOL(pMember->IsReadOnly());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezTestClass2")
  {
    ezTestClass2 Instance;
    const ezRTTI* pRtti = ezGetStaticRTTI<ezTestClass2>();

    {
      ezAbstractProperty* pProp = pRtti->FindPropertyByName("Text");
      EZ_TEST_BOOL(pProp != nullptr);

      EZ_TEST_BOOL(pProp->GetCategory() == ezAbstractProperty::Member);
      ezAbstractMemberProperty* pAbs = (ezAbstractMemberProperty*) pProp;

      EZ_TEST_BOOL(pAbs->GetPropertyType() == ezGetStaticRTTI<const char*>());
      ezTypedMemberProperty<const char*>* pMember = (ezTypedMemberProperty<const char*>*) pAbs;

      EZ_TEST_BOOL(!pMember->IsReadOnly());

      EZ_TEST_STRING(pMember->GetValue(&Instance), "Legen");

      pMember->SetValue(&Instance, "dary");

      EZ_TEST_STRING(pMember->GetValue(&Instance), "dary");
    }

    {
      ezAbstractProperty* pProp = pRtti->FindPropertyByName("Sub Vector", false);
      EZ_TEST_BOOL(pProp == nullptr);
    }

    {
      ezAbstractProperty* pProp = pRtti->FindPropertyByName("Sub Vector");
      EZ_TEST_BOOL(pProp != nullptr);

      EZ_TEST_BOOL(pProp->GetCategory() == ezAbstractProperty::Member);
      ezAbstractMemberProperty* pAbs = (ezAbstractMemberProperty*) pProp;

      EZ_TEST_BOOL(pAbs->GetPropertyType() == ezGetStaticRTTI<ezVec3>());
      ezTypedMemberProperty<ezVec3>* pMember = (ezTypedMemberProperty<ezVec3>*) pAbs;

      EZ_TEST_BOOL(pMember->IsReadOnly());

      EZ_TEST_VEC3(pMember->GetValue(&Instance), ezVec3(3, 4, 5), 0);
    }

    {
      ezAbstractProperty* pProp = pRtti->FindPropertyByName("Sub Struct", false);
      EZ_TEST_BOOL(pProp == nullptr);
    }

    {
      ezAbstractProperty* pProp = pRtti->FindPropertyByName("Sub Struct");
      EZ_TEST_BOOL(pProp != nullptr);

      EZ_TEST_BOOL(pProp->GetCategory() == ezAbstractProperty::Member);
      ezAbstractMemberProperty* pAbs = (ezAbstractMemberProperty*) pProp;

      const ezRTTI* pStruct = pAbs->GetPropertyType();
      void* pSubStruct = pAbs->GetPropertyPointer(&Instance);

      EZ_TEST_BOOL(pSubStruct != nullptr);

      ezAbstractProperty* pProp2 = pStruct->FindPropertyByName("Float");
      EZ_TEST_BOOL(pProp2 != nullptr);

      ezTypedMemberProperty<float>* pMember = (ezTypedMemberProperty<float>*) pProp2;
      EZ_TEST_FLOAT(pMember->GetValue(pSubStruct), 33.3f, 0.00001f);

      pMember->SetValue(pSubStruct, 44.4f);

      EZ_TEST_FLOAT(pMember->GetValue(pSubStruct), 44.4f, 0.00001f);
    }
  }

}

EZ_CREATE_SIMPLE_TEST(Reflection, ReflectionUtils)
{

  ezStringBuilder sOutputFolder1 = BUILDSYSTEM_OUTPUT_FOLDER;
  sOutputFolder1.AppendPath("FoundationTest", "Reflection");

  //ezOSFile osf;
  //osf.CreateDirectoryStructure(sOutputFolder1.GetData());

  //ezFileSystem::ClearAllDataDirectoryFactories();
  //ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  //EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder1.GetData(), ezFileSystem::AllowWrites, "Clear") == EZ_SUCCESS);

  ezMemoryStreamStorage StreamStorage;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "WriteObjectToJSON")
  {
    ezMemoryStreamWriter FileOut(&StreamStorage);

    //ezFileWriter FileOut;
    //EZ_TEST_BOOL(FileOut.Open("JSON.txt") == EZ_SUCCESS);

    ezTestClass2 c2;
    c2.SetText("Hallo");
    c2.m_MyVector.Set(14, 16, 18);
    c2.m_Struct.m_fFloat1 = 128;
    c2.m_Struct.m_UInt8 = 234;
    c2.m_Color.SetRGB(ezVec3(0.1f, 0.2f, 0.3f));
    c2.m_Time = ezTime::Seconds(91.0f);

    ezReflectionUtils::WriteObjectToJSON(FileOut, c2.GetDynamicRTTI(), &c2, ezJSONWriter::WhitespaceMode::All);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadObjectPropertiesFromJSON")
  {
    ezMemoryStreamReader FileIn(&StreamStorage);

    //ezFileReader FileIn;
    //EZ_TEST_BOOL(FileIn.Open("JSON.txt") == EZ_SUCCESS);

    ezTestClass2 c2;

    ezReflectionUtils::ReadObjectPropertiesFromJSON(FileIn, *c2.GetDynamicRTTI(), &c2);

    EZ_TEST_STRING(c2.GetText(), "Hallo");
    EZ_TEST_VEC3(c2.m_MyVector, ezVec3(3, 4, 5), 0.0f);
    EZ_TEST_FLOAT(c2.m_Time.GetSeconds(), 91.0f, 0.0f);
    EZ_TEST_VEC3(c2.m_Color.GetRGB<float>(), ezVec3(0.1f, 0.2f, 0.3f), 0.0f);
    EZ_TEST_FLOAT(c2.m_Struct.m_fFloat1, 128, 0.0f);
    EZ_TEST_INT(c2.m_Struct.m_UInt8, 234);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadObjectPropertiesFromJSON (different type)")
  {
    // here we restore the same properties into a different type of object which has properties that are named the same
    // but may have slightly different types (but which are compatible)

    ezMemoryStreamReader FileIn(&StreamStorage);

    //ezFileReader FileIn;
    //EZ_TEST_BOOL(FileIn.Open("JSON.txt") == EZ_SUCCESS);

    ezTestClass2b c2;

    ezReflectionUtils::ReadObjectPropertiesFromJSON(FileIn, *c2.GetDynamicRTTI(), &c2);

    EZ_TEST_STRING(c2.GetText(), "Tut"); // not restored, different property name
    EZ_TEST_VEC3(c2.m_Color.GetRGB<float>(), ezVec3(0.1f, 0.2f, 0.3f), 0.0f);
    EZ_TEST_FLOAT(c2.m_Struct.m_fFloat1, 128, 0.0f);
    EZ_TEST_INT(c2.m_Struct.m_UInt8, 234);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadObjectFromJSON")
  {
    ezMemoryStreamReader FileIn(&StreamStorage);

    //ezFileReader FileIn;
    //EZ_TEST_BOOL(FileIn.Open("JSON.txt") == EZ_SUCCESS);

    const ezRTTI* pRtti;
    void* pObject = ezReflectionUtils::ReadObjectFromJSON(FileIn, pRtti);

    ezTestClass2& c2 = *((ezTestClass2*) pObject);

    EZ_TEST_STRING(c2.GetText(), "Hallo");
    EZ_TEST_VEC3(c2.m_MyVector, ezVec3(3, 4, 5), 0.0f);
    EZ_TEST_FLOAT(c2.m_Time.GetSeconds(), 91.0f, 0.0f);
    EZ_TEST_VEC3(c2.m_Color.GetRGB<float>(), ezVec3(0.1f, 0.2f, 0.3f), 0.0f);
    EZ_TEST_FLOAT(c2.m_Struct.m_fFloat1, 128, 0.0f);
    EZ_TEST_INT(c2.m_Struct.m_UInt8, 234);

    if (pObject)
    {
      pRtti->GetAllocator()->Deallocate(pObject);
    }
  }

  ezFileSystem::ClearAllDataDirectories();
}




