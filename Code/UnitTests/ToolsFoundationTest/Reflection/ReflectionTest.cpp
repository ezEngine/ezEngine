#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Reflection);


void VariantToPropertyTest(void* pIntStruct, const ezRTTI* pRttiInt, const char* szPropName, ezVariant::Type::Enum type)
{
  ezAbstractMemberProperty* pProp = ezReflectionUtils::GetMemberProperty(pRttiInt, szPropName);
  EZ_TEST_BOOL(pProp != nullptr);
  if (pProp)
  {
    ezVariant oldValue = ezReflectionUtils::GetMemberPropertyValue(pProp, pIntStruct);
    EZ_TEST_BOOL(oldValue.IsValid());
    EZ_TEST_BOOL(oldValue.GetType() == type);

    ezVariant defaultValue = ezReflectionUtils::GetDefaultValue(pProp);
    EZ_TEST_BOOL(defaultValue.GetType() == type);
    ezReflectionUtils::SetMemberPropertyValue(pProp, pIntStruct, defaultValue);

    ezVariant newValue = ezReflectionUtils::GetMemberPropertyValue(pProp, pIntStruct);
    EZ_TEST_BOOL(newValue.IsValid());
    EZ_TEST_BOOL(newValue.GetType() == type);
    EZ_TEST_BOOL(newValue == defaultValue);
    EZ_TEST_BOOL(newValue != oldValue);
  }
}

EZ_CREATE_SIMPLE_TEST(Reflection, ReflectionUtils)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Integer Properties")
  {
    ezIntegerStruct intStruct;
    const ezRTTI* pRttiInt = ezRTTI::FindTypeByName("ezIntegerStruct");
    EZ_TEST_BOOL(pRttiInt != nullptr);

    VariantToPropertyTest(&intStruct, pRttiInt, "Int8", ezVariant::Type::Int8);
    EZ_TEST_INT(0, intStruct.GetInt8());
    VariantToPropertyTest(&intStruct, pRttiInt, "UInt8", ezVariant::Type::UInt8);
    EZ_TEST_INT(0, intStruct.GetUInt8());

    VariantToPropertyTest(&intStruct, pRttiInt, "Int16", ezVariant::Type::Int16);
    EZ_TEST_INT(0, intStruct.m_iInt16);
    VariantToPropertyTest(&intStruct, pRttiInt, "UInt16", ezVariant::Type::UInt16);
    EZ_TEST_INT(0, intStruct.m_iUInt16);

    VariantToPropertyTest(&intStruct, pRttiInt, "Int32", ezVariant::Type::Int32);
    EZ_TEST_INT(0, intStruct.GetInt32());
    VariantToPropertyTest(&intStruct, pRttiInt, "UInt32", ezVariant::Type::UInt32);
    EZ_TEST_INT(0, intStruct.GetUInt32());

    VariantToPropertyTest(&intStruct, pRttiInt, "Int64", ezVariant::Type::Int64);
    EZ_TEST_INT(0, intStruct.m_iInt64);
    VariantToPropertyTest(&intStruct, pRttiInt, "UInt64", ezVariant::Type::UInt64);
    EZ_TEST_INT(0, intStruct.m_iUInt64);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Float Properties")
  {
    ezFloatStruct floatStruct;
    const ezRTTI* pRttiFloat = ezRTTI::FindTypeByName("ezFloatStruct");
    EZ_TEST_BOOL(pRttiFloat != nullptr);

    VariantToPropertyTest(&floatStruct, pRttiFloat, "Float", ezVariant::Type::Float);
    EZ_TEST_FLOAT(0, floatStruct.GetFloat(), 0);
    VariantToPropertyTest(&floatStruct, pRttiFloat, "Double", ezVariant::Type::Double);
    EZ_TEST_FLOAT(0, floatStruct.GetDouble(), 0);
    VariantToPropertyTest(&floatStruct, pRttiFloat, "Time", ezVariant::Type::Time);
    EZ_TEST_FLOAT(0, floatStruct.GetTime().GetSeconds(), 0);
    VariantToPropertyTest(&floatStruct, pRttiFloat, "Angle", ezVariant::Type::Angle);
    EZ_TEST_FLOAT(0, floatStruct.GetAngle().GetDegree(), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Misc Properties")
  {
    ezPODClass podClass;
    const ezRTTI* pRttiPOD = ezRTTI::FindTypeByName("ezPODClass");
    EZ_TEST_BOOL(pRttiPOD != nullptr);

    VariantToPropertyTest(&podClass, pRttiPOD, "Bool", ezVariant::Type::Bool);
    EZ_TEST_BOOL(podClass.GetBool() == false);
    VariantToPropertyTest(&podClass, pRttiPOD, "Color", ezVariant::Type::Color);
    EZ_TEST_BOOL(podClass.GetColor() == ezColor(1.0f, 1.0f, 1.0f, 1.0f));
    VariantToPropertyTest(&podClass, pRttiPOD, "String", ezVariant::Type::String);
    EZ_TEST_STRING(podClass.GetString(), "");
    VariantToPropertyTest(&podClass, pRttiPOD, "Buffer", ezVariant::Type::DataBuffer);
    EZ_TEST_BOOL(podClass.GetBuffer() == ezDataBuffer());
    VariantToPropertyTest(&podClass, pRttiPOD, "VarianceAngle", ezVariant::Type::TypedObject);
    EZ_TEST_BOOL(podClass.GetCustom() == ezVarianceTypeAngle{});
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Math Properties")
  {
    ezMathClass mathClass;
    const ezRTTI* pRttiMath = ezRTTI::FindTypeByName("ezMathClass");
    EZ_TEST_BOOL(pRttiMath != nullptr);

    VariantToPropertyTest(&mathClass, pRttiMath, "Vec2", ezVariant::Type::Vector2);
    EZ_TEST_BOOL(mathClass.GetVec2() == ezVec2(0.0f, 0.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec3", ezVariant::Type::Vector3);
    EZ_TEST_BOOL(mathClass.GetVec3() == ezVec3(0.0f, 0.0f, 0.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec4", ezVariant::Type::Vector4);
    EZ_TEST_BOOL(mathClass.GetVec4() == ezVec4(0.0f, 0.0f, 0.0f, 0.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec2I", ezVariant::Type::Vector2I);
    EZ_TEST_BOOL(mathClass.m_Vec2I == ezVec2I32(0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec3I", ezVariant::Type::Vector3I);
    EZ_TEST_BOOL(mathClass.m_Vec3I == ezVec3I32(0, 0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec4I", ezVariant::Type::Vector4I);
    EZ_TEST_BOOL(mathClass.m_Vec4I == ezVec4I32(0, 0, 0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Quat", ezVariant::Type::Quaternion);
    EZ_TEST_BOOL(mathClass.GetQuat() == ezQuat(0.0f, 0.0f, 0.0f, 1.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Mat3", ezVariant::Type::Matrix3);
    EZ_TEST_BOOL(mathClass.GetMat3() == ezMat3::IdentityMatrix());
    VariantToPropertyTest(&mathClass, pRttiMath, "Mat4", ezVariant::Type::Matrix4);
    EZ_TEST_BOOL(mathClass.GetMat4() == ezMat4::IdentityMatrix());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Enumeration Properties")
  {
    ezEnumerationsClass enumClass;
    const ezRTTI* pRttiEnum = ezRTTI::FindTypeByName("ezEnumerationsClass");
    EZ_TEST_BOOL(pRttiEnum != nullptr);

    VariantToPropertyTest(&enumClass, pRttiEnum, "Enum", ezVariant::Type::Int64);
    EZ_TEST_BOOL(enumClass.GetEnum() == ezExampleEnum::Value1);
    VariantToPropertyTest(&enumClass, pRttiEnum, "Bitflags", ezVariant::Type::Int64);
    EZ_TEST_BOOL(enumClass.GetBitflags() == ezExampleBitflags::Value1);
  }
}

void AccessorPropertyTest(ezIReflectedTypeAccessor& ref_accessor, const char* szProperty, ezVariant::Type::Enum type)
{
  ezVariant oldValue = ref_accessor.GetValue(szProperty);
  EZ_TEST_BOOL(oldValue.IsValid());
  EZ_TEST_BOOL(oldValue.GetType() == type);

  ezAbstractProperty* pProp = ref_accessor.GetType()->FindPropertyByName(szProperty);
  ezVariant defaultValue = ezReflectionUtils::GetDefaultValue(pProp);
  EZ_TEST_BOOL(defaultValue.GetType() == type);
  bool bSetSuccess = ref_accessor.SetValue(szProperty, defaultValue);
  EZ_TEST_BOOL(bSetSuccess);

  ezVariant newValue = ref_accessor.GetValue(szProperty);
  EZ_TEST_BOOL(newValue.IsValid());
  EZ_TEST_BOOL(newValue.GetType() == type);
  EZ_TEST_BOOL(newValue == defaultValue);
}

ezUInt32 AccessorPropertiesTest(ezIReflectedTypeAccessor& ref_accessor, const ezRTTI* pType)
{
  ezUInt32 uiPropertiesSet = 0;
  EZ_TEST_BOOL(pType != nullptr);

  // Call for base class
  if (pType->GetParentType() != nullptr)
  {
    uiPropertiesSet += AccessorPropertiesTest(ref_accessor, pType->GetParentType());
  }

  // Test properties
  ezUInt32 uiPropCount = pType->GetProperties().GetCount();
  for (ezUInt32 i = 0; i < uiPropCount; ++i)
  {
    ezAbstractProperty* pProp = pType->GetProperties()[i];
    const bool bIsValueType = ezReflectionUtils::IsValueType(pProp);

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Member:
      {
        ezAbstractMemberProperty* pProp3 = static_cast<ezAbstractMemberProperty*>(pProp);
        if (pProp->GetFlags().IsSet(ezPropertyFlags::IsEnum))
        {
          AccessorPropertyTest(ref_accessor, pProp->GetPropertyName(), ezVariant::Type::Int64);
          uiPropertiesSet++;
        }
        else if (pProp->GetFlags().IsSet(ezPropertyFlags::Bitflags))
        {
          AccessorPropertyTest(ref_accessor, pProp->GetPropertyName(), ezVariant::Type::Int64);
          uiPropertiesSet++;
        }
        else if (bIsValueType)
        {
          AccessorPropertyTest(ref_accessor, pProp->GetPropertyName(), pProp3->GetSpecificType()->GetVariantType());
          uiPropertiesSet++;
        }
        else // ezPropertyFlags::Class
        {
          // Recurs into sub-classes
          const ezUuid& subObjectGuid = ref_accessor.GetValue(pProp->GetPropertyName()).Get<ezUuid>();
          ezDocumentObject* pEmbeddedClassObject = const_cast<ezDocumentObject*>(ref_accessor.GetOwner()->GetChild(subObjectGuid));
          uiPropertiesSet += AccessorPropertiesTest(pEmbeddedClassObject->GetTypeAccessor(), pProp3->GetSpecificType());
        }
      }
      break;
      case ezPropertyCategory::Array:
      {
        // ezAbstractArrayProperty* pProp3 = static_cast<ezAbstractArrayProperty*>(pProp);
        // TODO
      }
      break;

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }
  return uiPropertiesSet;
}

ezUInt32 AccessorPropertiesTest(ezIReflectedTypeAccessor& ref_accessor)
{
  const ezRTTI* handle = ref_accessor.GetType();
  return AccessorPropertiesTest(ref_accessor, handle);
}

static ezUInt32 GetTypeCount()
{
  ezUInt32 uiCount = 0;
  ezRTTI::ForEachType([&](const ezRTTI* pRtti) { uiCount++; });
  return uiCount;
}

static const ezRTTI* RegisterType(const char* szTypeName)
{
  const ezRTTI* pRtti = ezRTTI::FindTypeByName(szTypeName);
  EZ_TEST_BOOL(pRtti != nullptr);

  ezReflectedTypeDescriptor desc;
  ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRtti, desc);
  return ezPhantomRttiManager::RegisterType(desc);
}

EZ_CREATE_SIMPLE_TEST(Reflection, ReflectedType)
{
  ezTestDocumentObjectManager manager;

  /*const ezRTTI* pRttiBase =*/RegisterType("ezReflectedClass");
  /*const ezRTTI* pRttiEnumBase =*/RegisterType("ezEnumBase");
  /*const ezRTTI* pRttiBitflagsBase =*/RegisterType("ezBitflagsBase");

  const ezRTTI* pRttiInt = RegisterType("ezIntegerStruct");
  const ezRTTI* pRttiFloat = RegisterType("ezFloatStruct");
  const ezRTTI* pRttiPOD = RegisterType("ezPODClass");
  const ezRTTI* pRttiMath = RegisterType("ezMathClass");
  /*const ezRTTI* pRttiEnum =*/RegisterType("ezExampleEnum");
  /*const ezRTTI* pRttiFlags =*/RegisterType("ezExampleBitflags");
  const ezRTTI* pRttiEnumerations = RegisterType("ezEnumerationsClass");

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezReflectedTypeStorageAccessor")
  {
    {
      ezDocumentObject* pObject = manager.CreateObject(pRttiInt);
      EZ_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 8);
      manager.DestroyObject(pObject);
    }
    {
      ezDocumentObject* pObject = manager.CreateObject(pRttiFloat);
      EZ_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 4);
      manager.DestroyObject(pObject);
    }
    {
      ezDocumentObject* pObject = manager.CreateObject(pRttiPOD);
      EZ_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 18);
      manager.DestroyObject(pObject);
    }
    {
      ezDocumentObject* pObject = manager.CreateObject(pRttiMath);
      EZ_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 27);
      manager.DestroyObject(pObject);
    }
    {
      ezDocumentObject* pObject = manager.CreateObject(pRttiEnumerations);
      EZ_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 2);
      manager.DestroyObject(pObject);
    }
  }
}


EZ_CREATE_SIMPLE_TEST(Reflection, ReflectedTypeReloading)
{
  ezTestDocumentObjectManager manager;

  const ezRTTI* pRttiInner = ezRTTI::FindTypeByName("InnerStruct");
  const ezRTTI* pRttiInnerP = nullptr;
  ezReflectedTypeDescriptor descInner;

  const ezRTTI* pRttiOuter = ezRTTI::FindTypeByName("OuterClass");
  const ezRTTI* pRttiOuterP = nullptr;
  ezReflectedTypeDescriptor descOuter;

  ezUInt32 uiRegisteredBaseTypes = GetTypeCount();
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RegisterType")
  {
    EZ_TEST_BOOL(pRttiInner != nullptr);
    ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiInner, descInner);
    descInner.m_sTypeName = "InnerStructP";
    pRttiInnerP = ezPhantomRttiManager::RegisterType(descInner);
    EZ_TEST_BOOL(pRttiInnerP != nullptr);

    EZ_TEST_BOOL(pRttiOuter != nullptr);
    ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiOuter, descOuter);
    descOuter.m_sTypeName = "OuterClassP";
    descOuter.m_Properties[0].m_sType = "InnerStructP";
    pRttiOuterP = ezPhantomRttiManager::RegisterType(descOuter);
    EZ_TEST_BOOL(pRttiOuterP != nullptr);
  }

  {
    ezDocumentObject* pInnerObject = manager.CreateObject(pRttiInnerP);
    manager.AddObject(pInnerObject, nullptr, "Children", -1);
    ezIReflectedTypeAccessor& innerAccessor = pInnerObject->GetTypeAccessor();

    ezDocumentObject* pOuterObject = manager.CreateObject(pRttiOuterP);
    manager.AddObject(pOuterObject, nullptr, "Children", -1);
    ezIReflectedTypeAccessor& outerAccessor = pOuterObject->GetTypeAccessor();

    ezUuid innerGuid = outerAccessor.GetValue("Inner").Get<ezUuid>();
    ezDocumentObject* pEmbeddedInnerObject = manager.GetObject(innerGuid);
    ezIReflectedTypeAccessor& embeddedInnerAccessor = pEmbeddedInnerObject->GetTypeAccessor();

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetValues")
    {
      // Just set a few values to make sure they don't get messed up by the following operations.
      EZ_TEST_BOOL(innerAccessor.SetValue("IP1", 1.4f));
      EZ_TEST_BOOL(outerAccessor.SetValue("OP1", 0.9f));
      EZ_TEST_BOOL(embeddedInnerAccessor.SetValue("IP1", 1.4f));
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddProperty")
    {
      // Say we reload the engine and the InnerStruct now has a second property: IP2.
      descInner.m_Properties.PushBack(ezReflectedPropertyDescriptor(ezPropertyCategory::Member, "IP2", "ezVec4",
        ezBitflags<ezPropertyFlags>(ezPropertyFlags::StandardType), ezArrayPtr<ezPropertyAttribute* const>()));
      const ezRTTI* NewInnerHandle = ezPhantomRttiManager::RegisterType(descInner);
      EZ_TEST_BOOL(NewInnerHandle == pRttiInnerP);

      // Check that the new property is present.
      AccessorPropertyTest(innerAccessor, "IP2", ezVariant::Type::Vector4);

      AccessorPropertyTest(embeddedInnerAccessor, "IP2", ezVariant::Type::Vector4);

      // Test that the old properties are still valid.
      EZ_TEST_BOOL(innerAccessor.GetValue("IP1") == 1.4f);
      EZ_TEST_BOOL(outerAccessor.GetValue("OP1") == 0.9f);
      EZ_TEST_BOOL(embeddedInnerAccessor.GetValue("IP1") == 1.4f);
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "ChangeProperty")
    {
      // Out original inner float now is a Int32!
      descInner.m_Properties[0].m_sType = "ezInt32";
      const ezRTTI* NewInnerHandle = ezPhantomRttiManager::RegisterType(descInner);
      EZ_TEST_BOOL(NewInnerHandle == pRttiInnerP);

      // Test if the previous value was converted correctly to its new type.
      ezVariant innerValue = innerAccessor.GetValue("IP1");
      EZ_TEST_BOOL(innerValue.IsValid());
      EZ_TEST_BOOL(innerValue.GetType() == ezVariant::Type::Int32);
      EZ_TEST_INT(innerValue.Get<ezInt32>(), 1);

      ezVariant outerValue = embeddedInnerAccessor.GetValue("IP1");
      EZ_TEST_BOOL(outerValue.IsValid());
      EZ_TEST_BOOL(outerValue.GetType() == ezVariant::Type::Int32);
      EZ_TEST_INT(outerValue.Get<ezInt32>(), 1);

      // Test that the old properties are still valid.
      EZ_TEST_BOOL(outerAccessor.GetValue("OP1") == 0.9f);

      AccessorPropertyTest(innerAccessor, "IP2", ezVariant::Type::Vector4);
      AccessorPropertyTest(embeddedInnerAccessor, "IP2", ezVariant::Type::Vector4);
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "DeleteProperty")
    {
      // Lets now delete the original inner property IP1.
      descInner.m_Properties.RemoveAtAndCopy(0);
      const ezRTTI* NewInnerHandle = ezPhantomRttiManager::RegisterType(descInner);
      EZ_TEST_BOOL(NewInnerHandle == pRttiInnerP);

      // Check that IP1 is really gone.
      EZ_TEST_BOOL(!innerAccessor.GetValue("IP1").IsValid());
      EZ_TEST_BOOL(!embeddedInnerAccessor.GetValue("IP1").IsValid());

      // Test that the old properties are still valid.
      EZ_TEST_BOOL(outerAccessor.GetValue("OP1") == 0.9f);

      AccessorPropertyTest(innerAccessor, "IP2", ezVariant::Type::Vector4);
      AccessorPropertyTest(embeddedInnerAccessor, "IP2", ezVariant::Type::Vector4);
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "RevertProperties")
    {
      // Reset all classes to their initial state.
      ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiInner, descInner);
      descInner.m_sTypeName = "InnerStructP";
      ezPhantomRttiManager::RegisterType(descInner);

      ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiOuter, descOuter);
      descInner.m_sTypeName = "OuterStructP";
      descOuter.m_Properties[0].m_sType = "InnerStructP";
      ezPhantomRttiManager::RegisterType(descOuter);

      // Test that the old properties are back again.
      ezStringBuilder path = "IP1";
      ezVariant innerValue = innerAccessor.GetValue(path);
      EZ_TEST_BOOL(innerValue.IsValid());
      EZ_TEST_BOOL(innerValue.GetType() == ezVariant::Type::Float);
      EZ_TEST_FLOAT(innerValue.Get<float>(), 1.0f, 0.0f);

      ezVariant outerValue = embeddedInnerAccessor.GetValue("IP1");
      EZ_TEST_BOOL(outerValue.IsValid());
      EZ_TEST_BOOL(outerValue.GetType() == ezVariant::Type::Float);
      EZ_TEST_FLOAT(outerValue.Get<float>(), 1.0f, 0.0f);
      EZ_TEST_BOOL(outerAccessor.GetValue("OP1") == 0.9f);
    }

    manager.RemoveObject(pInnerObject);
    manager.DestroyObject(pInnerObject);

    manager.RemoveObject(pOuterObject);
    manager.DestroyObject(pOuterObject);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "UnregisterType")
  {
    EZ_TEST_INT(GetTypeCount(), uiRegisteredBaseTypes + 2);
    ezPhantomRttiManager::UnregisterType(pRttiOuterP);
    ezPhantomRttiManager::UnregisterType(pRttiInnerP);
    EZ_TEST_INT(GetTypeCount(), uiRegisteredBaseTypes);
  }
}
