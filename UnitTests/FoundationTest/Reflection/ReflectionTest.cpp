#include <PCH.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>


EZ_CREATE_SIMPLE_TEST_GROUP(Reflection);


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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TypeFlags")
  {
    EZ_TEST_INT(ezGetStaticRTTI<bool>()->GetTypeFlags().GetValue(), ezTypeFlags::StandardType);
    EZ_TEST_INT(ezGetStaticRTTI<ezUuid>()->GetTypeFlags().GetValue(), ezTypeFlags::StandardType);
    EZ_TEST_INT(ezGetStaticRTTI<const char*>()->GetTypeFlags().GetValue(), ezTypeFlags::StandardType);
    EZ_TEST_INT(ezGetStaticRTTI<ezString>()->GetTypeFlags().GetValue(), ezTypeFlags::StandardType);
    EZ_TEST_INT(ezGetStaticRTTI<ezMat4>()->GetTypeFlags().GetValue(), ezTypeFlags::StandardType);
    EZ_TEST_INT(ezGetStaticRTTI<ezVariant>()->GetTypeFlags().GetValue(), ezTypeFlags::StandardType);

    EZ_TEST_INT(ezGetStaticRTTI<ezAbstractTestClass>()->GetTypeFlags().GetValue(), ezTypeFlags::Abstract);
    EZ_TEST_INT(ezGetStaticRTTI<ezAbstractTestStruct>()->GetTypeFlags().GetValue(), ezTypeFlags::Abstract);

    EZ_TEST_INT(ezGetStaticRTTI<ezTestStruct3>()->GetTypeFlags().GetValue(), ezTypeFlags::Default);
    EZ_TEST_INT(ezGetStaticRTTI<ezTestClass2>()->GetTypeFlags().GetValue(), ezTypeFlags::Default);

    EZ_TEST_INT(ezGetStaticRTTI<ezExampleEnum>()->GetTypeFlags().GetValue(), ezTypeFlags::IsEnum);
    EZ_TEST_INT(ezGetStaticRTTI<ezExampleBitflags>()->GetTypeFlags().GetValue(), ezTypeFlags::Bitflags);
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
      EZ_TEST_INT(Props.GetCount(), 5);
      EZ_TEST_STRING(Props[0]->GetPropertyName(), "Float");
      EZ_TEST_STRING(Props[1]->GetPropertyName(), "Vector");
      EZ_TEST_STRING(Props[2]->GetPropertyName(), "Int");
      EZ_TEST_STRING(Props[3]->GetPropertyName(), "UInt8");
      EZ_TEST_STRING(Props[4]->GetPropertyName(), "Variant");
    }

    {
      ezRTTI* pType = ezRTTI::FindTypeByName("ezTestClass2");

      auto Props = pType->GetProperties();
      EZ_TEST_INT(Props.GetCount(), 6);
      EZ_TEST_STRING(Props[0]->GetPropertyName(), "Text");
      EZ_TEST_STRING(Props[1]->GetPropertyName(), "Time");
      EZ_TEST_STRING(Props[2]->GetPropertyName(), "Enum");
      EZ_TEST_STRING(Props[3]->GetPropertyName(), "Bitflags");
      EZ_TEST_STRING(Props[4]->GetPropertyName(), "Array");
      EZ_TEST_STRING(Props[5]->GetPropertyName(), "Variant");

      ezHybridArray<ezAbstractProperty*, 32> AllProps;
      pType->GetAllProperties(AllProps);

      EZ_TEST_INT(AllProps.GetCount(), 9);
      EZ_TEST_STRING(AllProps[0]->GetPropertyName(), "Sub Struct");
      EZ_TEST_STRING(AllProps[1]->GetPropertyName(), "Color");
      EZ_TEST_STRING(AllProps[2]->GetPropertyName(), "Sub Vector");
      EZ_TEST_STRING(AllProps[3]->GetPropertyName(), "Text");
      EZ_TEST_STRING(AllProps[4]->GetPropertyName(), "Time");
      EZ_TEST_STRING(AllProps[5]->GetPropertyName(), "Enum");
      EZ_TEST_STRING(AllProps[6]->GetPropertyName(), "Bitflags");
      EZ_TEST_STRING(AllProps[7]->GetPropertyName(), "Array");
      EZ_TEST_STRING(AllProps[8]->GetPropertyName(), "Variant");
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

        EZ_TEST_BOOL(pProp->GetCategory() == ezPropertyCategory::Member);
        ezAbstractMemberProperty* pAbsMember = (ezAbstractMemberProperty*) pProp;

        EZ_TEST_BOOL(pAbsMember->GetSpecificType() == ezGetStaticRTTI<float>());

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
    EZ_TEST_BOOL(pRtti->GetVariantType() == ezVariant::Type::Invalid);

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
    EZ_TEST_BOOL(pRtti->GetVariantType() == ezVariant::Type::Invalid);

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


template<typename T, typename T2>
void TestMemberProperty(const char* szPropName, void* pObject, const ezRTTI* pRtti, ezBitflags<ezPropertyFlags> expectedFlags, T2 expectedValue, T2 testValue)
{
  ezAbstractProperty* pProp = pRtti->FindPropertyByName(szPropName);
  EZ_TEST_BOOL(pProp != nullptr);

  EZ_TEST_BOOL(pProp->GetCategory() == ezPropertyCategory::Member);

  EZ_TEST_BOOL(pProp->GetSpecificType() == ezGetStaticRTTI<T>());
  ezTypedMemberProperty<T>* pMember = (ezTypedMemberProperty<T>*) pProp;

  EZ_TEST_INT(pMember->GetFlags().GetValue(), expectedFlags.GetValue());

  T value = pMember->GetValue(pObject);
  EZ_TEST_BOOL(expectedValue == value);

  if (!pMember->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
  {
    pMember->SetValue(pObject, testValue);

    EZ_TEST_BOOL(testValue == pMember->GetValue(pObject));
  }
}

EZ_CREATE_SIMPLE_TEST(Reflection, MemberProperties)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezTestStruct")
  {
    ezTestStruct data;
    const ezRTTI* pRtti = ezGetStaticRTTI<ezTestStruct>();

    TestMemberProperty<float>("Float", &data, pRtti, ezPropertyFlags::StandardType, 1.1f, 5.0f);
    TestMemberProperty<ezInt32>("Int", &data, pRtti, ezPropertyFlags::StandardType, 2, -8);
    TestMemberProperty<ezVec3>("Vector", &data, pRtti, ezPropertyFlags::StandardType | ezPropertyFlags::ReadOnly, ezVec3(3, 4, 5), ezVec3(0, -1.0f, 3.14f));
    TestMemberProperty<ezVariant>("Variant", &data, pRtti, ezPropertyFlags::StandardType, ezVariant("Test"), ezVariant(ezVec3(0, -1.0f, 3.14f)));

  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezTestClass2")
  {
    ezTestClass2 Instance;
    const ezRTTI* pRtti = ezGetStaticRTTI<ezTestClass2>();

    {
      TestMemberProperty<const char*>("Text", &Instance, pRtti, ezPropertyFlags::StandardType, ezString("Legen"), ezString("dary"));
      ezAbstractProperty* pProp = pRtti->FindPropertyByName("Sub Vector", false);
      EZ_TEST_BOOL(pProp == nullptr);
    }

    { 
      TestMemberProperty<ezVec3>("Sub Vector", &Instance, pRtti, ezPropertyFlags::StandardType | ezPropertyFlags::ReadOnly, ezVec3(3, 4, 5), ezVec3(3, 4, 5));
      ezAbstractProperty* pProp = pRtti->FindPropertyByName("Sub Struct", false);
      EZ_TEST_BOOL(pProp == nullptr);
    }

    {
      ezAbstractProperty* pProp = pRtti->FindPropertyByName("Sub Struct");
      EZ_TEST_BOOL(pProp != nullptr);

      EZ_TEST_BOOL(pProp->GetCategory() == ezPropertyCategory::Member);
      ezAbstractMemberProperty* pAbs = (ezAbstractMemberProperty*) pProp;

      const ezRTTI* pStruct = pAbs->GetSpecificType();
      void* pSubStruct = pAbs->GetPropertyPointer(&Instance);

      EZ_TEST_BOOL(pSubStruct != nullptr);

      TestMemberProperty<float>("Float", pSubStruct, pStruct, ezPropertyFlags::StandardType, 33.3f, 44.4f);
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
    c2.m_Color = ezColor(0.1f, 0.2f, 0.3f);
    c2.m_Time = ezTime::Seconds(91.0f);
    c2.m_enumClass = ezExampleEnum::Value3;
    c2.m_bitflagsClass = ezExampleBitflags::Enum(ezExampleBitflags::Value1 | ezExampleBitflags::Value2 | ezExampleBitflags::Value3);
    c2.m_array.PushBack(5.0f);
    c2.m_array.PushBack(10.0f);
    c2.m_Variant = ezVec3(1.0f, 2.0f, 3.0f);

    ezReflectionSerializer::WriteObjectToJSON(FileOut, c2.GetDynamicRTTI(), &c2, ezJSONWriter::WhitespaceMode::All);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadObjectPropertiesFromJSON")
  {
    ezMemoryStreamReader FileIn(&StreamStorage);

    //ezFileReader FileIn;
    //EZ_TEST_BOOL(FileIn.Open("JSON.txt") == EZ_SUCCESS);

    ezTestClass2 c2;

    ezReflectionSerializer::ReadObjectPropertiesFromJSON(FileIn, *c2.GetDynamicRTTI(), &c2);

    EZ_TEST_STRING(c2.GetText(), "Hallo");
    EZ_TEST_VEC3(c2.m_MyVector, ezVec3(3, 4, 5), 0.0f);
    EZ_TEST_FLOAT(c2.m_Time.GetSeconds(), 91.0f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Color.r, 0.1f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Color.g, 0.2f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Color.b, 0.3f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Struct.m_fFloat1, 128, 0.0f);
    EZ_TEST_INT(c2.m_Struct.m_UInt8, 234);
    EZ_TEST_BOOL(c2.m_enumClass == ezExampleEnum::Value3);
    EZ_TEST_BOOL(c2.m_bitflagsClass == ezExampleBitflags::Enum(ezExampleBitflags::Value1 | ezExampleBitflags::Value2 | ezExampleBitflags::Value3));
    EZ_TEST_INT(c2.m_array.GetCount(), 2);
    if (c2.m_array.GetCount() == 2)
    {
      EZ_TEST_FLOAT(c2.m_array[0], 5.0f, 0.0f);
      EZ_TEST_FLOAT(c2.m_array[1], 10.0f, 0.0f);
    }
    EZ_TEST_VEC3(c2.m_Variant.Get<ezVec3>(), ezVec3(1.0f, 2.0f, 3.0f), 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadObjectPropertiesFromJSON (different type)")
  {
    // here we restore the same properties into a different type of object which has properties that are named the same
    // but may have slightly different types (but which are compatible)

    ezMemoryStreamReader FileIn(&StreamStorage);

    //ezFileReader FileIn;
    //EZ_TEST_BOOL(FileIn.Open("JSON.txt") == EZ_SUCCESS);

    ezTestClass2b c2;

    ezReflectionSerializer::ReadObjectPropertiesFromJSON(FileIn, *c2.GetDynamicRTTI(), &c2);

    EZ_TEST_STRING(c2.GetText(), "Tut"); // not restored, different property name
    EZ_TEST_FLOAT(c2.m_Color.r, 0.1f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Color.g, 0.2f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Color.b, 0.3f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Struct.m_fFloat1, 128, 0.0f);
    EZ_TEST_INT(c2.m_Struct.m_UInt8, 234);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadObjectFromJSON")
  {
    ezMemoryStreamReader FileIn(&StreamStorage);

    //ezFileReader FileIn;
    //EZ_TEST_BOOL(FileIn.Open("JSON.txt") == EZ_SUCCESS);

    const ezRTTI* pRtti;
    void* pObject = ezReflectionSerializer::ReadObjectFromJSON(FileIn, pRtti);

    ezTestClass2& c2 = *((ezTestClass2*) pObject);

    EZ_TEST_STRING(c2.GetText(), "Hallo");
    EZ_TEST_VEC3(c2.m_MyVector, ezVec3(3, 4, 5), 0.0f);
    EZ_TEST_FLOAT(c2.m_Time.GetSeconds(), 91.0f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Color.r, 0.1f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Color.g, 0.2f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Color.b, 0.3f, 0.0f);
    EZ_TEST_FLOAT(c2.m_Struct.m_fFloat1, 128, 0.0f);
    EZ_TEST_INT(c2.m_Struct.m_UInt8, 234);
    EZ_TEST_BOOL(c2.m_enumClass == ezExampleEnum::Value3);
    EZ_TEST_BOOL(c2.m_bitflagsClass == ezExampleBitflags::Enum(ezExampleBitflags::Value1 | ezExampleBitflags::Value2 | ezExampleBitflags::Value3));
    EZ_TEST_INT(c2.m_array.GetCount(), 2);
    if (c2.m_array.GetCount() == 2)
    {
      EZ_TEST_FLOAT(c2.m_array[0], 5.0f, 0.0f);
      EZ_TEST_FLOAT(c2.m_array[1], 10.0f, 0.0f);
    }
    EZ_TEST_VEC3(c2.m_Variant.Get<ezVec3>(), ezVec3(1.0f, 2.0f, 3.0f), 0.0f);

    if (pObject)
    {
      pRtti->GetAllocator()->Deallocate(pObject);
    }
  }

  ezFileSystem::ClearAllDataDirectories();
}


EZ_CREATE_SIMPLE_TEST(Reflection, Enum)
{
  const ezRTTI* pEnumRTTI = ezGetStaticRTTI<ezExampleEnum>();
  const ezRTTI* pRTTI = ezGetStaticRTTI<ezTestEnumStruct>();

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Enum Constants")
  {
    EZ_TEST_BOOL(pEnumRTTI->IsDerivedFrom<ezEnumBase>());
    auto props = pEnumRTTI->GetProperties();
    EZ_TEST_INT(props.GetCount(), 4);  // Default + 3

    for (auto pProp : props)
    {
      EZ_TEST_BOOL(pProp->GetCategory() == ezPropertyCategory::Constant);
      ezAbstractConstantProperty* pConstantProp = static_cast<ezAbstractConstantProperty*>(pProp);
      EZ_TEST_BOOL(pConstantProp->GetSpecificType() == ezGetStaticRTTI<ezInt8>());
    }
    EZ_TEST_INT(ezExampleEnum::Default, ezReflectionUtils::DefaultEnumerationValue(pEnumRTTI));

    EZ_TEST_STRING(props[0]->GetPropertyName(), "ezExampleEnum::Default");
    EZ_TEST_STRING(props[1]->GetPropertyName(), "ezExampleEnum::Value1");
    EZ_TEST_STRING(props[2]->GetPropertyName(), "ezExampleEnum::Value2");
    EZ_TEST_STRING(props[3]->GetPropertyName(), "ezExampleEnum::Value3");

    auto pTypedConstantProp0 = static_cast<ezTypedConstantProperty<ezInt8>*>(props[0]);
    auto pTypedConstantProp1 = static_cast<ezTypedConstantProperty<ezInt8>*>(props[1]);
    auto pTypedConstantProp2 = static_cast<ezTypedConstantProperty<ezInt8>*>(props[2]);
    auto pTypedConstantProp3 = static_cast<ezTypedConstantProperty<ezInt8>*>(props[3]);
    EZ_TEST_INT(pTypedConstantProp0->GetValue(), ezExampleEnum::Default);
    EZ_TEST_INT(pTypedConstantProp1->GetValue(), ezExampleEnum::Value1);
    EZ_TEST_INT(pTypedConstantProp2->GetValue(), ezExampleEnum::Value2);
    EZ_TEST_INT(pTypedConstantProp3->GetValue(), ezExampleEnum::Value3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Enum Property")
  {
    ezTestEnumStruct data;
    auto props = pRTTI->GetProperties();
    EZ_TEST_INT(props.GetCount(), 4);

    for (auto pProp : props)
    {
      EZ_TEST_BOOL(pProp->GetCategory() == ezPropertyCategory::Member);
      ezAbstractMemberProperty* pMemberProp = static_cast<ezAbstractMemberProperty*>(pProp);
      EZ_TEST_INT(pMemberProp->GetFlags().GetValue(), ezPropertyFlags::IsEnum);
      EZ_TEST_BOOL(pMemberProp->GetSpecificType() == pEnumRTTI);
      ezAbstractEnumerationProperty* pEnumProp = static_cast<ezAbstractEnumerationProperty*>(pProp);
      EZ_TEST_BOOL(pEnumProp->GetValue(&data) == ezExampleEnum::Value1);

      const ezRTTI* pEnumPropertyRTTI = pEnumProp->GetSpecificType();
      // Set and get all valid enum values.
      for (auto pProp2 : pEnumPropertyRTTI->GetProperties().GetSubArray(1))
      {
        ezTypedConstantProperty<ezInt8>* pConstantProp = static_cast<ezTypedConstantProperty<ezInt8>*>(pProp2);
        pEnumProp->SetValue(&data, pConstantProp->GetValue());
        EZ_TEST_INT(pEnumProp->GetValue(&data), pConstantProp->GetValue());

        // Enum <-> string
        ezStringBuilder sValue;
        EZ_TEST_BOOL(ezReflectionUtils::EnumerationToString(pEnumPropertyRTTI, pConstantProp->GetValue(), sValue));
        EZ_TEST_STRING(sValue, pConstantProp->GetPropertyName());

        // Setting the value via a string also works.
        pEnumProp->SetValue(&data, ezExampleEnum::Value1);
        ezReflectionUtils::SetMemberPropertyValue(pEnumProp, &data, sValue.GetData());
        EZ_TEST_INT(pEnumProp->GetValue(&data), pConstantProp->GetValue());

        ezInt64 iValue = 0;
        EZ_TEST_BOOL(ezReflectionUtils::StringToEnumeration(pEnumPropertyRTTI, sValue, iValue));
        EZ_TEST_INT(iValue, pConstantProp->GetValue());

        EZ_TEST_INT(iValue, ezReflectionUtils::MakeEnumerationValid(pEnumPropertyRTTI, iValue));
        EZ_TEST_INT(ezExampleEnum::Default, ezReflectionUtils::MakeEnumerationValid(pEnumPropertyRTTI, iValue + 666));
      }
    }

    EZ_TEST_BOOL(data.m_enum == ezExampleEnum::Value3);
    EZ_TEST_BOOL(data.m_enumClass == ezExampleEnum::Value3);

    EZ_TEST_BOOL(data.GetEnum() == ezExampleEnum::Value3);
    EZ_TEST_BOOL(data.GetEnumClass() == ezExampleEnum::Value3);
  }
}


EZ_CREATE_SIMPLE_TEST(Reflection, Bitflags)
{
  const ezRTTI* pBitflagsRTTI = ezGetStaticRTTI<ezExampleBitflags>();
  const ezRTTI* pRTTI = ezGetStaticRTTI<ezTestBitflagsStruct>();

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Bitflags Constants")
  {
    EZ_TEST_BOOL(pBitflagsRTTI->IsDerivedFrom<ezBitflagsBase>());
    auto props = pBitflagsRTTI->GetProperties();
    EZ_TEST_INT(props.GetCount(), 4); // Default + 3

    for (auto pProp : props)
    {
      EZ_TEST_BOOL(pProp->GetCategory() == ezPropertyCategory::Constant);
      EZ_TEST_BOOL(pProp->GetSpecificType() == ezGetStaticRTTI<ezUInt64>());
    }
    EZ_TEST_INT(ezExampleBitflags::Default, ezReflectionUtils::DefaultEnumerationValue(pBitflagsRTTI));

    EZ_TEST_STRING(props[0]->GetPropertyName(), "ezExampleBitflags::Default");
    EZ_TEST_STRING(props[1]->GetPropertyName(), "ezExampleBitflags::Value1");
    EZ_TEST_STRING(props[2]->GetPropertyName(), "ezExampleBitflags::Value2");
    EZ_TEST_STRING(props[3]->GetPropertyName(), "ezExampleBitflags::Value3");

    auto pTypedConstantProp0 = static_cast<ezTypedConstantProperty<ezUInt64>*>(props[0]);
    auto pTypedConstantProp1 = static_cast<ezTypedConstantProperty<ezUInt64>*>(props[1]);
    auto pTypedConstantProp2 = static_cast<ezTypedConstantProperty<ezUInt64>*>(props[2]);
    auto pTypedConstantProp3 = static_cast<ezTypedConstantProperty<ezUInt64>*>(props[3]);
    EZ_TEST_BOOL(pTypedConstantProp0->GetValue() == ezExampleBitflags::Default);
    EZ_TEST_BOOL(pTypedConstantProp1->GetValue() == ezExampleBitflags::Value1);
    EZ_TEST_BOOL(pTypedConstantProp2->GetValue() == ezExampleBitflags::Value2);
    EZ_TEST_BOOL(pTypedConstantProp3->GetValue() == ezExampleBitflags::Value3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Bitflags Property")
  {
    ezTestBitflagsStruct data;
    auto props = pRTTI->GetProperties();
    EZ_TEST_INT(props.GetCount(), 2);

    for (auto pProp : props)
    {
      EZ_TEST_BOOL(pProp->GetCategory() == ezPropertyCategory::Member);
      EZ_TEST_BOOL(pProp->GetSpecificType() == pBitflagsRTTI);
      EZ_TEST_INT(pProp->GetFlags().GetValue(), ezPropertyFlags::Bitflags);
      ezAbstractEnumerationProperty* pBitflagsProp = static_cast<ezAbstractEnumerationProperty*>(pProp);
      EZ_TEST_BOOL(pBitflagsProp->GetValue(&data) == ezExampleBitflags::Value1);

      const ezRTTI* pBitflagsPropertyRTTI = pBitflagsProp->GetSpecificType();

      // Set and get all valid bitflags values. (skip default value)
      ezUInt64 constants[] = { static_cast<ezTypedConstantProperty<ezUInt64>*>(pBitflagsPropertyRTTI->GetProperties()[1])->GetValue(),
        static_cast<ezTypedConstantProperty<ezUInt64>*>(pBitflagsPropertyRTTI->GetProperties()[2])->GetValue(),
        static_cast<ezTypedConstantProperty<ezUInt64>*>(pBitflagsPropertyRTTI->GetProperties()[3])->GetValue() };

      const char* stringValues[] = {"", "ezExampleBitflags::Value1", "ezExampleBitflags::Value2",
        "ezExampleBitflags::Value1|ezExampleBitflags::Value2", "ezExampleBitflags::Value3",
        "ezExampleBitflags::Value1|ezExampleBitflags::Value3", "ezExampleBitflags::Value2|ezExampleBitflags::Value3",
        "ezExampleBitflags::Value1|ezExampleBitflags::Value2|ezExampleBitflags::Value3"};
      for (ezInt32 i = 0; i < 8; ++i)
      {
        ezUInt64 uiBitflagValue = 0;
        uiBitflagValue |= (i & EZ_BIT(0)) != 0 ? constants[0] : 0;
        uiBitflagValue |= (i & EZ_BIT(1)) != 0 ? constants[1] : 0;
        uiBitflagValue |= (i & EZ_BIT(2)) != 0 ? constants[2] : 0;

        pBitflagsProp->SetValue(&data, uiBitflagValue);
        EZ_TEST_INT(pBitflagsProp->GetValue(&data), uiBitflagValue);

        // Bitflags <-> string
        ezStringBuilder sValue;
        EZ_TEST_BOOL(ezReflectionUtils::EnumerationToString(pBitflagsPropertyRTTI, uiBitflagValue, sValue));
        EZ_TEST_STRING(sValue, stringValues[i]);

        // Setting the value via a string also works.
        pBitflagsProp->SetValue(&data, 0);
        ezReflectionUtils::SetMemberPropertyValue(pBitflagsProp, &data, sValue.GetData());
        EZ_TEST_INT(pBitflagsProp->GetValue(&data), uiBitflagValue);

        ezInt64 iValue = 0;
        EZ_TEST_BOOL(ezReflectionUtils::StringToEnumeration(pBitflagsPropertyRTTI, sValue, iValue));
        EZ_TEST_INT(iValue, uiBitflagValue);

        EZ_TEST_INT(iValue, ezReflectionUtils::MakeEnumerationValid(pBitflagsPropertyRTTI, iValue));
        EZ_TEST_INT(iValue, ezReflectionUtils::MakeEnumerationValid(pBitflagsPropertyRTTI, iValue | EZ_BIT(16)));
      }
    }

    EZ_TEST_BOOL(data.m_bitflagsClass == (ezExampleBitflags::Value1 | ezExampleBitflags::Value2 | ezExampleBitflags::Value3));
    EZ_TEST_BOOL(data.GetBitflagsClass() == (ezExampleBitflags::Value1 | ezExampleBitflags::Value2 | ezExampleBitflags::Value3));
  }
}


template<typename T>
void TestArrayProperty(const char* szPropName, void* pObject, const ezRTTI* pRtti, T& value)
{
  ezAbstractProperty* pProp = pRtti->FindPropertyByName(szPropName);
  EZ_TEST_BOOL(pProp != nullptr);
  EZ_TEST_BOOL(pProp->GetCategory() == ezPropertyCategory::Array);
  ezAbstractArrayProperty* pArrayProp = static_cast<ezAbstractArrayProperty*>(pProp);
  const ezRTTI* pElemRtti = pProp->GetSpecificType();
  EZ_TEST_BOOL(pElemRtti == ezGetStaticRTTI<T>());

  if (!pArrayProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
  {
    // If we don't know the element type T but we can allocate it, we can handle it anyway.
    if (pElemRtti->GetAllocator()->CanAllocate())
    {
      void* pData = pElemRtti->GetAllocator()->Allocate();

      pArrayProp->SetCount(pObject, 2);
      EZ_TEST_INT(pArrayProp->GetCount(pObject), 2);
      // Push default constructed object in both slots.
      pArrayProp->SetValue(pObject, 0, pData);
      pArrayProp->SetValue(pObject, 1, pData);

      // Retrieve it again and compare to function parameter, they should be different.
      pArrayProp->GetValue(pObject, 0, pData);
      EZ_TEST_BOOL(*static_cast<T*>(pData) != value);
      pArrayProp->GetValue(pObject, 1, pData);
      EZ_TEST_BOOL(*static_cast<T*>(pData) != value);

      pElemRtti->GetAllocator()->Deallocate(pData);
    }

    pArrayProp->Clear(pObject);
    EZ_TEST_INT(pArrayProp->GetCount(pObject), 0);
    pArrayProp->SetCount(pObject, 2);
    pArrayProp->SetValue(pObject, 0, &value);
    pArrayProp->SetValue(pObject, 1, &value);

    // Insert default init values
    T temp;
    pArrayProp->Insert(pObject, 2, &temp);
    EZ_TEST_INT(pArrayProp->GetCount(pObject), 3);
    pArrayProp->Insert(pObject, 0, &temp);
    EZ_TEST_INT(pArrayProp->GetCount(pObject), 4);

    // Remove them again
    pArrayProp->Remove(pObject, 3);
    EZ_TEST_INT(pArrayProp->GetCount(pObject), 3);
    pArrayProp->Remove(pObject, 0);
    EZ_TEST_INT(pArrayProp->GetCount(pObject), 2);
  }

  // Assumes this function gets called first by a writeable property, and then immediately by the same data as a read-only property.
  // So the checks are valid for the read-only version, too.
  EZ_TEST_INT(pArrayProp->GetCount(pObject), 2);

  T v1;
  pArrayProp->GetValue(pObject, 0, &v1);
  EZ_TEST_BOOL(v1 == value);

  T v2;
  pArrayProp->GetValue(pObject, 1, &v2);
  EZ_TEST_BOOL(v2 == value);

  if (pElemRtti->GetAllocator()->CanAllocate())
  {
    // Current values should be different from default constructed version.
    void* pData = pElemRtti->GetAllocator()->Allocate();

    EZ_TEST_BOOL(*static_cast<T*>(pData) != v1);
    EZ_TEST_BOOL(*static_cast<T*>(pData) != v2);

    pElemRtti->GetAllocator()->Deallocate(pData);
  }
}

EZ_CREATE_SIMPLE_TEST(Reflection, Arrays)
{
  ezTestArrays containers;
  const ezRTTI* pRtti = ezGetStaticRTTI<ezTestArrays>();
  EZ_TEST_BOOL(pRtti != nullptr);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "POD Array")
  {
    double fValue = 5;
    TestArrayProperty<double>("Hybrid", &containers, pRtti, fValue);
    TestArrayProperty<double>("HybridRO", &containers, pRtti, fValue);

    TestArrayProperty<double>("AcHybrid", &containers, pRtti, fValue);
    TestArrayProperty<double>("AcHybridRO", &containers, pRtti, fValue);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Struct Array")
  {
    ezTestStruct3 data;
    data.m_fFloat1 = 99.0f;
    data.m_UInt8 = 127;

    TestArrayProperty<ezTestStruct3>("Dynamic", &containers, pRtti, data);
    TestArrayProperty<ezTestStruct3>("DynamicRO", &containers, pRtti, data);

    TestArrayProperty<ezTestStruct3>("AcDynamic", &containers, pRtti, data);
    TestArrayProperty<ezTestStruct3>("AcDynamicRO", &containers, pRtti, data);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezReflectedClass Array")
  {
    ezTestArrays data;
    data.m_Hybrid.PushBack(42.0);

    TestArrayProperty<ezTestArrays>("Deque", &containers, pRtti, data);
    TestArrayProperty<ezTestArrays>("DequeRO", &containers, pRtti, data);

    TestArrayProperty<ezTestArrays>("AcDeque", &containers, pRtti, data);
    TestArrayProperty<ezTestArrays>("AcDequeRO", &containers, pRtti, data);
  }

  ezMemoryStreamStorage StreamStorage;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "WriteObjectToJSON")
  {
    ezMemoryStreamWriter FileOut(&StreamStorage);

    ezReflectionSerializer::WriteObjectToJSON(FileOut, containers.GetDynamicRTTI(), &containers, ezJSONWriter::WhitespaceMode::All);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadObjectPropertiesFromJSON")
  {
    ezMemoryStreamReader FileIn(&StreamStorage);
    ezTestArrays data;
    ezReflectionSerializer::ReadObjectPropertiesFromJSON(FileIn, *data.GetDynamicRTTI(), &data);

    EZ_TEST_BOOL(data == containers);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadObjectFromJSON")
  {
    ezMemoryStreamReader FileIn(&StreamStorage);

    const ezRTTI* pRtti;
    void* pObject = ezReflectionSerializer::ReadObjectFromJSON(FileIn, pRtti);

    ezTestArrays& c2 = *((ezTestArrays*) pObject);

    EZ_TEST_BOOL(c2 == containers);

    if (pObject)
    {
      pRtti->GetAllocator()->Deallocate(pObject);
    }
  }
}


template<typename T>
void TestSetProperty(const char* szPropName, void* pObject, const ezRTTI* pRtti, T& value1, T& value2)
{
  ezAbstractProperty* pProp = pRtti->FindPropertyByName(szPropName);
  EZ_TEST_BOOL(pProp != nullptr);
  EZ_TEST_BOOL(pProp->GetCategory() == ezPropertyCategory::Set);
  ezAbstractSetProperty* pSetProp = static_cast<ezAbstractSetProperty*>(pProp);
  const ezRTTI* pElemRtti = pProp->GetSpecificType();
  EZ_TEST_BOOL(pElemRtti == ezGetStaticRTTI<T>());
  EZ_TEST_BOOL(ezReflectionUtils::IsBasicType(pElemRtti));

  if (!pSetProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
  {
    pSetProp->Clear(pObject);
    EZ_TEST_INT(pSetProp->GetCount(pObject), 0);
    pSetProp->Insert(pObject, &value1);
    EZ_TEST_INT(pSetProp->GetCount(pObject), 1);
    EZ_TEST_BOOL(pSetProp->Contains(pObject, &value1));
    EZ_TEST_BOOL(!pSetProp->Contains(pObject, &value2));
    pSetProp->Insert(pObject, &value2);
    EZ_TEST_INT(pSetProp->GetCount(pObject), 2);
    EZ_TEST_BOOL(pSetProp->Contains(pObject, &value1));
    EZ_TEST_BOOL(pSetProp->Contains(pObject, &value2));

    // Insert default init value
    if (!ezIsPointer<T>::value)
    {
      T temp;
      pSetProp->Insert(pObject, &temp);
      EZ_TEST_INT(pSetProp->GetCount(pObject), 3);
      EZ_TEST_BOOL(pSetProp->Contains(pObject, &value1));
      EZ_TEST_BOOL(pSetProp->Contains(pObject, &value2));
      EZ_TEST_BOOL(pSetProp->Contains(pObject, &temp));

      // Remove it again
      pSetProp->Remove(pObject, &temp);
      EZ_TEST_INT(pSetProp->GetCount(pObject), 2);
      EZ_TEST_BOOL(!pSetProp->Contains(pObject, &temp));
    }
  }

  // Assumes this function gets called first by a writeable property, and then immediately by the same data as a read-only property.
  // So the checks are valid for the read-only version, too.
  EZ_TEST_INT(pSetProp->GetCount(pObject), 2);
  EZ_TEST_BOOL(pSetProp->Contains(pObject, &value1));
  EZ_TEST_BOOL(pSetProp->Contains(pObject, &value2));


  ezHybridArray<ezVariant, 16> keys;
  pSetProp->GetValues(pObject, keys);
  EZ_TEST_INT(keys.GetCount(), 2);

}

EZ_CREATE_SIMPLE_TEST(Reflection, Sets)
{
  ezTestSets containers;
  const ezRTTI* pRtti = ezGetStaticRTTI<ezTestSets>();
  EZ_TEST_BOOL(pRtti != nullptr);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezSet")
  {
    ezInt8 iValue1 = -5;
    ezInt8 iValue2 = 127;
    TestSetProperty<ezInt8>("Set", &containers, pRtti, iValue1, iValue2);
    TestSetProperty<ezInt8>("SetRO", &containers, pRtti, iValue1, iValue2);

    double fValue1 = 5;
    double fValue2 = -3;
    TestSetProperty<double>("AcSet", &containers, pRtti, fValue1, fValue2);
    TestSetProperty<double>("AcSetRO", &containers, pRtti, fValue1, fValue2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezDeque Pseudo Set")
  {
    int iValue1 = -5;
    int iValue2 = 127;

    TestSetProperty<int>("AcPseudoSet", &containers, pRtti, iValue1, iValue2);
    TestSetProperty<int>("AcPseudoSetRO", &containers, pRtti, iValue1, iValue2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezSetPtr Pseudo Set")
  {
    ezString sValue1 = "TestString1";
    ezString sValue2 = "Test String Deus";

    TestSetProperty<ezString>("AcPseudoSet2", &containers, pRtti, sValue1, sValue2);
    TestSetProperty<ezString>("AcPseudoSet2RO", &containers, pRtti, sValue1, sValue2);

    const char* szValue1 = "TestString1";
    const char* szValue2 = "Test String Deus";
    TestSetProperty<const char*>("AcPseudoSet2b", &containers, pRtti, szValue1, szValue2);
  }

  ezMemoryStreamStorage StreamStorage;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "WriteObjectToJSON")
  {
    ezMemoryStreamWriter FileOut(&StreamStorage);

    ezReflectionSerializer::WriteObjectToJSON(FileOut, containers.GetDynamicRTTI(), &containers, ezJSONWriter::WhitespaceMode::All);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadObjectPropertiesFromJSON")
  {
    ezMemoryStreamReader FileIn(&StreamStorage);
    ezTestSets data;
    ezReflectionSerializer::ReadObjectPropertiesFromJSON(FileIn, *data.GetDynamicRTTI(), &data);

    EZ_TEST_BOOL(data == containers);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadObjectFromJSON")
  {
    ezMemoryStreamReader FileIn(&StreamStorage);

    const ezRTTI* pRtti;
    void* pObject = ezReflectionSerializer::ReadObjectFromJSON(FileIn, pRtti);

    ezTestSets& c2 = *((ezTestSets*) pObject);

    EZ_TEST_BOOL(c2 == containers);

    if (pObject)
    {
      pRtti->GetAllocator()->Deallocate(pObject);
    }
  }
}


template<typename T>
void TestPointerMemberProperty(const char* szPropName, void* pObject, const ezRTTI* pRtti, ezBitflags<ezPropertyFlags> expectedFlags, T* pExpectedValue)
{
  ezAbstractProperty* pProp = pRtti->FindPropertyByName(szPropName);
  EZ_TEST_BOOL(pProp != nullptr);
  EZ_TEST_BOOL(pProp->GetCategory() == ezPropertyCategory::Member);
  ezAbstractMemberProperty* pAbsMember = (ezAbstractMemberProperty*) pProp;
  EZ_TEST_INT(pProp->GetFlags().GetValue(), expectedFlags.GetValue());
  EZ_TEST_BOOL(pProp->GetSpecificType() == ezGetStaticRTTI<T>());
  void* pData = nullptr;
  pAbsMember->GetValuePtr(pObject, &pData);
  EZ_TEST_BOOL(pData == pExpectedValue);

  // Set value to null.
  {
    void* pDataNull = nullptr;
    pAbsMember->SetValuePtr(pObject, &pDataNull);
    void* pDataNull2 = nullptr;
    pAbsMember->GetValuePtr(pObject, &pDataNull2);
    EZ_TEST_BOOL(pDataNull == pDataNull2);
  }

  // Set value to new instance.
  {
    void* pNewData = pAbsMember->GetSpecificType()->GetAllocator()->Allocate();
    pAbsMember->SetValuePtr(pObject, &pNewData);
    void* pData2 = nullptr;
    pAbsMember->GetValuePtr(pObject, &pData2);
    EZ_TEST_BOOL(pNewData == pData2);

  }

  // Delete old value
  pAbsMember->GetSpecificType()->GetAllocator()->Deallocate(pData);
}

EZ_CREATE_SIMPLE_TEST(Reflection, Pointer)
{
  const ezRTTI* pRtti = ezGetStaticRTTI<ezTestPtr>();
  EZ_TEST_BOOL(pRtti != nullptr);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Member Property Ptr")
  {
    ezTestPtr containers;
    {
      ezAbstractProperty* pProp = pRtti->FindPropertyByName("ConstCharPtr");
      EZ_TEST_BOOL(pProp != nullptr);
      EZ_TEST_BOOL(pProp->GetCategory() == ezPropertyCategory::Member);
      EZ_TEST_INT(pProp->GetFlags().GetValue(), ezPropertyFlags::StandardType);
      EZ_TEST_BOOL(pProp->GetSpecificType() == ezGetStaticRTTI<const char*>());
    }

    TestPointerMemberProperty<ezTestArrays>("ArraysPtr", &containers, pRtti, ezPropertyFlags::Pointer | ezPropertyFlags::PointerOwner, containers.m_pArrays);
    TestPointerMemberProperty<ezTestArrays>("ArraysPtrDirect", &containers, pRtti, ezPropertyFlags::Pointer | ezPropertyFlags::PointerOwner, containers.m_pArraysDirect);
  }

  ezTestPtr containers;
  ezMemoryStreamStorage StreamStorage;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Serialize Property Ptr")
  {
    containers.m_sString = "Test";

    containers.m_pArrays = EZ_DEFAULT_NEW(ezTestArrays);
    containers.m_pArrays->m_Deque.PushBack(ezTestArrays());

    containers.m_ArrayPtr.PushBack(EZ_DEFAULT_NEW(ezTestArrays));
    containers.m_ArrayPtr[0]->m_Hybrid.PushBack(5.0);

    containers.m_SetPtr.Insert(EZ_DEFAULT_NEW(ezTestSets));
    containers.m_SetPtr.GetIterator().Key()->m_Array.PushBack("BLA");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "WriteObjectToJSON")
  {
    ezMemoryStreamWriter FileOut(&StreamStorage);

    ezReflectionSerializer::WriteObjectToJSON(FileOut, containers.GetDynamicRTTI(), &containers, ezJSONWriter::WhitespaceMode::All);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadObjectPropertiesFromJSON")
  {
    ezMemoryStreamReader FileIn(&StreamStorage);
    ezTestPtr data;
    ezReflectionSerializer::ReadObjectPropertiesFromJSON(FileIn, *data.GetDynamicRTTI(), &data);

    EZ_TEST_BOOL(data.m_sString == containers.m_sString);
    EZ_TEST_BOOL(*data.m_pArrays == *containers.m_pArrays);
    EZ_TEST_BOOL(*data.m_ArrayPtr[0] == *containers.m_ArrayPtr[0]);
    EZ_TEST_BOOL(*data.m_SetPtr.GetIterator().Key() == *containers.m_SetPtr.GetIterator().Key());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadObjectFromJSON")
  {
    ezMemoryStreamReader FileIn(&StreamStorage);

    const ezRTTI* pRtti2;
    void* pObject = ezReflectionSerializer::ReadObjectFromJSON(FileIn, pRtti2);
    EZ_TEST_BOOL(pRtti == pRtti2);
    
    ezTestPtr& data = *((ezTestPtr*) pObject);

    EZ_TEST_BOOL(data.m_sString == containers.m_sString);
    EZ_TEST_BOOL(*data.m_pArrays == *containers.m_pArrays);
    EZ_TEST_BOOL(*data.m_ArrayPtr[0] == *containers.m_ArrayPtr[0]);
    EZ_TEST_BOOL(*data.m_SetPtr.GetIterator().Key() == *containers.m_SetPtr.GetIterator().Key());

    if (pObject)
    {
      pRtti->GetAllocator()->Deallocate(pObject);
    }
  }

}
