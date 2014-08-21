#include <PCH.h>
#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Reflection);


struct ezIntegerStruct
{
public:
  ezIntegerStruct()
  {
    m_iInt8 = 1;
    m_iUInt8 = 1;
    m_iInt16 = 1;
    m_iUInt16 = 1;
    m_iInt32 = 1;
    m_iUInt32 = 1;
    m_iInt64 = 1;
    m_iUInt64 = 1;
  }

  void SetInt8(ezInt8 i) { m_iInt8 = i; }
  ezInt8 GetInt8() const { return m_iInt8; }
  void SetUInt8(ezUInt8 i) { m_iUInt8 = i; }
  ezUInt8 GetUInt8() const { return m_iUInt8; }
  void SetInt32(ezInt32 i) { m_iInt32 = i; }
  ezInt32 GetInt32() const { return m_iInt32; }
  void SetUInt32(ezUInt32 i) { m_iUInt32 = i; }
  ezUInt32 GetUInt32() const { return m_iUInt32; }

  ezInt16 m_iInt16;
  ezUInt16 m_iUInt16;
  ezInt64 m_iInt64;
  ezUInt64 m_iUInt64;

private:
  ezInt8 m_iInt8;
  ezUInt8 m_iUInt8;
  ezInt32 m_iInt32;
  ezUInt32 m_iUInt32;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezIntegerStruct);

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezIntegerStruct, ezNoBase, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Int8", GetInt8, SetInt8),
    EZ_ACCESSOR_PROPERTY("UInt8", GetUInt8, SetUInt8),
    EZ_MEMBER_PROPERTY("Int16", m_iInt16),
    EZ_MEMBER_PROPERTY("UInt16", m_iUInt16),
    EZ_ACCESSOR_PROPERTY("Int32", GetInt32, SetInt32),
    EZ_ACCESSOR_PROPERTY("UInt32", GetUInt32, SetUInt32),
    EZ_MEMBER_PROPERTY("Int64", m_iInt64),
    EZ_MEMBER_PROPERTY("UInt64", m_iUInt64)
  EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();



struct ezFloatStruct
{
public:
  ezFloatStruct()
  {
    m_fFloat = 1.0f;
    m_fDouble = 1.0;
    m_Time = ezTime::Seconds(1.0);
  }

  void SetFloat(float f) { m_fFloat = f; }
  float GetFloat() const { return m_fFloat; }
  void SetDouble(double d) { m_fDouble = d; }
  double GetDouble() const { return m_fDouble; }
  void SetTime(ezTime t) { m_Time = t; }
  ezTime GetTime() const { return m_Time; }

private:
  float m_fFloat;
  double m_fDouble;
  ezTime m_Time;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezFloatStruct);

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezFloatStruct, ezNoBase, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Float", GetFloat, SetFloat),
    EZ_ACCESSOR_PROPERTY("Double", GetDouble, SetDouble),
    EZ_ACCESSOR_PROPERTY("Time", GetTime, SetTime)
  EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();



class ezPODClass : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPODClass);

public:
  ezPODClass()
  {
    m_bBool = true;
    m_Color = ezColor(1.0f, 0.0f, 0.0f, 0.0f);
    m_sString = "Test";
  }

  ezIntegerStruct m_IntegerStruct;
  ezFloatStruct m_FloatStruct;

  void SetBool(bool b) { m_bBool = b; }
  bool GetBool() const { return m_bBool; }
  void SetColor(ezColor c) { m_Color = c; }
  ezColor GetColor() const { return m_Color; }
  const char* GetString() const { return m_sString.GetData(); }
  void SetString(const char* sz) { m_sString = sz; }

private:
  bool m_bBool;
  ezColor m_Color;
  ezString m_sString;
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPODClass, ezReflectedClass, 1, ezRTTIDefaultAllocator<ezPODClass>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Integer", m_IntegerStruct),
    EZ_MEMBER_PROPERTY("Float", m_FloatStruct),
    EZ_ACCESSOR_PROPERTY("Bool", GetBool, SetBool),
    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor),
    EZ_ACCESSOR_PROPERTY("String", GetString, SetString)
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

class ezMathClass : public ezPODClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMathClass);

public:
  ezMathClass()
  {
    m_Vec2 = ezVec2(1.0f, 1.0f);
    m_Vec3 = ezVec3(1.0f, 1.0f, 1.0f);
    m_Vec4 = ezVec4(1.0f, 1.0f, 1.0f, 1.0f);
    m_Quat = ezQuat(1.0f, 1.0f, 1.0f, 1.0f);
    m_Mat3.SetZero();
    m_Mat4.SetZero();
  }

  void SetVec2(ezVec2 v) { m_Vec2 = v; }
  ezVec2 GetVec2() const { return m_Vec2; }
  void SetVec3(ezVec3 v) { m_Vec3 = v; }
  ezVec3 GetVec3() const { return m_Vec3; }
  void SetVec4(ezVec4 v) { m_Vec4 = v; }
  ezVec4 GetVec4() const { return m_Vec4; }
  void SetQuat(ezQuat q) { m_Quat = q; }
  ezQuat GetQuat() const { return m_Quat; }
  void SetMat3(ezMat3 m) { m_Mat3 = m; }
  ezMat3 GetMat3() const { return m_Mat3; }
  void SetMat4(ezMat4 m) { m_Mat4 = m; }
  ezMat4 GetMat4() const { return m_Mat4; }

private:
  ezVec2 m_Vec2;
  ezVec3 m_Vec3;
  ezVec4 m_Vec4;
  ezQuat m_Quat;
  ezMat3 m_Mat3;
  ezMat4 m_Mat4;
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMathClass, ezPODClass, 1, ezRTTIDefaultAllocator<ezMathClass>);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Vec2", GetVec2, SetVec2),
    EZ_ACCESSOR_PROPERTY("Vec3", GetVec3, SetVec3),
    EZ_ACCESSOR_PROPERTY("Vec4", GetVec4, SetVec4),
    EZ_ACCESSOR_PROPERTY("Quat", GetQuat, SetQuat),
    EZ_ACCESSOR_PROPERTY("Mat3", GetMat3, SetMat3),
    EZ_ACCESSOR_PROPERTY("Mat4", GetMat4, SetMat4)
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();


void VariantToPropertyTest(void* intStruct, const ezRTTI* pRttiInt, const char* szPropName, ezVariant::Type::Enum type)
{
  ezAbstractMemberProperty* pProp = ezReflectionUtils::GetMemberProperty(pRttiInt, szPropName);
  EZ_TEST_BOOL(pProp != nullptr);
  if (pProp)
  {
    ezVariant oldValue = ezReflectionUtils::GetMemberPropertyValue(pProp, intStruct);
    EZ_TEST_BOOL(oldValue.IsValid());
    EZ_TEST_BOOL(oldValue.GetType() == type);

    ezVariant defaultValue = ezToolsReflectionUtils::GetDefaultVariantFromType(type);
    ezReflectionUtils::SetMemberPropertyValue(pProp, intStruct, defaultValue);
    
    ezVariant newValue = ezReflectionUtils::GetMemberPropertyValue(pProp, intStruct);
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
    ezRTTI* pRttiFloat = ezRTTI::FindTypeByName("ezFloatStruct");
    EZ_TEST_BOOL(pRttiFloat != nullptr);

    VariantToPropertyTest(&floatStruct, pRttiFloat, "Float", ezVariant::Type::Float);
    EZ_TEST_FLOAT(0, floatStruct.GetFloat(), 0);
    VariantToPropertyTest(&floatStruct, pRttiFloat, "Double", ezVariant::Type::Double);
    EZ_TEST_FLOAT(0, floatStruct.GetDouble(), 0);
    VariantToPropertyTest(&floatStruct, pRttiFloat, "Time", ezVariant::Type::Time);
    EZ_TEST_FLOAT(0, floatStruct.GetTime().GetSeconds(), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Misc Properties")
  {
    ezPODClass podClass;
    ezRTTI* pRttiPOD = ezRTTI::FindTypeByName("ezPODClass");
    EZ_TEST_BOOL(pRttiPOD != nullptr);

    VariantToPropertyTest(&podClass, pRttiPOD, "Bool", ezVariant::Type::Bool);
    EZ_TEST_BOOL(podClass.GetBool() == false);
    VariantToPropertyTest(&podClass, pRttiPOD, "Color", ezVariant::Type::Color);
    EZ_TEST_BOOL(podClass.GetColor() == ezColor(1.0f, 1.0f, 1.0f, 1.0f));
    VariantToPropertyTest(&podClass, pRttiPOD, "String", ezVariant::Type::String);
    EZ_TEST_STRING(podClass.GetString(), "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Math Properties")
  {
    ezMathClass mathClass;
    ezRTTI* pRttiMath = ezRTTI::FindTypeByName("ezMathClass");
    EZ_TEST_BOOL(pRttiMath != nullptr);

    VariantToPropertyTest(&mathClass, pRttiMath, "Vec2", ezVariant::Type::Vector2);
    EZ_TEST_BOOL(mathClass.GetVec2() == ezVec2(0.0f, 0.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec3", ezVariant::Type::Vector3);
    EZ_TEST_BOOL(mathClass.GetVec3() == ezVec3(0.0f, 0.0f, 0.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec4", ezVariant::Type::Vector4);
    EZ_TEST_BOOL(mathClass.GetVec4() == ezVec4(0.0f, 0.0f, 0.0f, 0.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Quat", ezVariant::Type::Quaternion);
    EZ_TEST_BOOL(mathClass.GetQuat() == ezQuat(0.0f, 0.0f, 0.0f, 1.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Mat3", ezVariant::Type::Matrix3);
    EZ_TEST_BOOL(mathClass.GetMat3() == ezMat3::IdentityMatrix());
    VariantToPropertyTest(&mathClass, pRttiMath, "Mat4", ezVariant::Type::Matrix4);
    EZ_TEST_BOOL(mathClass.GetMat4() == ezMat4::IdentityMatrix());
  }
}

ezReflectedTypeHandle RegisterTypeViaRtti(const ezRTTI* pRtti)
{
  // Gather properties
  EZ_TEST_BOOL(pRtti != nullptr);
  ezReflectedTypeDescriptor desc;
  ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRtti, desc);

  // Register type
  const ezUInt32 uiTypeCount = ezReflectedTypeManager::GetTypeCount();
  ezReflectedTypeHandle handle = ezReflectedTypeManager::RegisterType(desc);
  EZ_TEST_INT(ezReflectedTypeManager::GetTypeCount(), uiTypeCount + 1);
  EZ_TEST_BOOL(!handle.IsInvalidated());

  // Type / handle lookups tests
  ezReflectedTypeHandle handleViaManager = ezReflectedTypeManager::GetTypeHandleByName(pRtti->GetTypeName());
  EZ_TEST_BOOL(!handleViaManager.IsInvalidated());
  EZ_TEST_BOOL(handle == handleViaManager);

  const ezReflectedType* pType = ezReflectedTypeManager::GetType(handle);
  EZ_TEST_BOOL(pType != nullptr);
  EZ_TEST_BOOL(pType == handle.GetType());
  EZ_TEST_BOOL(handle == pType->GetTypeHandle());

  ReflectedTypeTable::ConstIterator it = ezReflectedTypeManager::GetTypeIterator();
  EZ_TEST_BOOL(it.IsValid());
  bool bFound = false;
  while (it.IsValid())
  {
    if (it.Value() == pType)
    {
      bFound = true;
      break;
    }
    it.Next();
  }
  EZ_TEST_BOOL_MSG(bFound, "The class was not found in the reflected type collection!");
 
  // Parent handle / type
  const ezRTTI* pParentRtti = pRtti->GetParentType();
  ezReflectedTypeHandle parentHandle = pType->GetParentTypeHandle();
  if (pParentRtti == nullptr)
  {
    EZ_TEST_BOOL(parentHandle.IsInvalidated());
  }
  else
  {
    EZ_TEST_BOOL(!parentHandle.IsInvalidated());
    EZ_TEST_BOOL(parentHandle == ezReflectedTypeManager::GetTypeHandleByName(pParentRtti->GetTypeName()));
  }

  // Type parameter tests
  EZ_TEST_STRING(pType->GetPluginName().GetString().GetData(), pRtti->GetPluginName());
  EZ_TEST_STRING(pType->GetTypeName().GetString().GetData(), pRtti->GetTypeName());

  // Type properties test
  EZ_TEST_INT(pType->GetPropertyCount(), desc.m_Properties.GetCount());

  const ezArrayPtr<ezAbstractProperty*>& rttiProps = pRtti->GetProperties();
  const ezUInt32 uiPropertyCount = rttiProps.GetCount();
  ezUInt32 uiFoundProperties = 0;
  for (ezUInt32 i = 0; i < uiPropertyCount; ++i)
  {
    ezAbstractProperty* prop = rttiProps[i];
    if (prop->GetCategory() == ezAbstractProperty::Member)
    {
      uiFoundProperties++;
      ezAbstractMemberProperty* memberProp = static_cast<ezAbstractMemberProperty*>(prop);
      const ezReflectedProperty* pReflectedProperty = pType->GetPropertyByName(memberProp->GetPropertyName());
      EZ_TEST_BOOL(pReflectedProperty != nullptr);
      EZ_TEST_STRING(memberProp->GetPropertyName(), pReflectedProperty->m_sPropertyName.GetString().GetData());
      EZ_TEST_BOOL(memberProp->IsReadOnly() == pReflectedProperty->m_Flags.IsSet(PropertyFlags::IsReadOnly));
      if (memberProp->GetPropertyType()->GetVariantType() == ezVariant::Type::Invalid)
      {
        EZ_TEST_BOOL(pReflectedProperty->m_Type == ezVariant::Type::Invalid);
        EZ_TEST_BOOL(!pReflectedProperty->m_hTypeHandle.IsInvalidated());
        EZ_TEST_BOOL(pReflectedProperty->m_hTypeHandle.GetType() != nullptr);
      }
      else
      {
        EZ_TEST_BOOL(pReflectedProperty->m_Type == memberProp->GetPropertyType()->GetVariantType());
        EZ_TEST_BOOL(pReflectedProperty->m_hTypeHandle.IsInvalidated());
      }
    }
  }
  EZ_TEST_INT(pType->GetPropertyCount(), uiFoundProperties);

  return handle;
}


void AccessorPropertyTest(ezIReflectedTypeAccessor& accessor, const ezPropertyPath& path, ezVariant::Type::Enum type)
{
  ezVariant oldValue = accessor.GetValue(path);
  EZ_TEST_BOOL(oldValue.IsValid());
  EZ_TEST_BOOL(oldValue.GetType() == type);

  ezVariant defaultValue = ezToolsReflectionUtils::GetDefaultVariantFromType(type);
  bool bSetSuccess = accessor.SetValue(path, defaultValue);
  EZ_TEST_BOOL(bSetSuccess);
    
  ezVariant newValue = accessor.GetValue(path);
  EZ_TEST_BOOL(newValue.IsValid());
  EZ_TEST_BOOL(newValue.GetType() == type);
  EZ_TEST_BOOL(newValue == defaultValue);
}

ezUInt32 AccessorPropertiesTest(ezIReflectedTypeAccessor& accessor, ezReflectedTypeHandle handle, ezPropertyPath& path)
{
  ezUInt32 uiPropertiesSet = 0;
  EZ_TEST_BOOL(!handle.IsInvalidated());
  const ezReflectedType* pType = handle.GetType();
  EZ_TEST_BOOL(pType != nullptr);

  // Call for base class
  if (!pType->GetParentTypeHandle().IsInvalidated())
  {
    uiPropertiesSet += AccessorPropertiesTest(accessor, pType->GetParentTypeHandle(), path);
  }

  // Test properties
  ezUInt32 uiPropCount = pType->GetPropertyCount();
  for (ezUInt32 i = 0; i < uiPropCount; ++i)
  {
    const ezReflectedProperty* pProp = pType->GetPropertyByIndex(i);
    // Build path to property
    ezPropertyPath propPath = path;
    propPath.PushBack(pProp->m_sPropertyName.GetString().GetData());

    if (pProp->m_Type == ezVariant::Type::Invalid)
    {
      // Recurs into sub-classes
      uiPropertiesSet += AccessorPropertiesTest(accessor, pProp->m_hTypeHandle, propPath);
    }
    else if (pProp->m_Type >= ezVariant::Type::Bool && pProp->m_Type <= ezVariant::Type::Time)
    {
      AccessorPropertyTest(accessor, propPath, pProp->m_Type);
      uiPropertiesSet++;
    }
    else
    {
      EZ_ASSERT(false, "Arrays and pointers are not tested yet!");
    }
  }
  return uiPropertiesSet;
}

ezUInt32 AccessorPropertiesTest(ezIReflectedTypeAccessor& accessor)
{
  ezReflectedTypeHandle handle = accessor.GetReflectedTypeHandle();
  ezPropertyPath path = ezPropertyPath();
  return AccessorPropertiesTest(accessor, handle, path);
}

EZ_CREATE_SIMPLE_TEST(Reflection, ReflectedType)
{
  const ezRTTI* pRttiBase = ezRTTI::FindTypeByName("ezReflectedClass");
  ezReflectedTypeHandle baseHandle;

  const ezRTTI* pRttiInt = ezRTTI::FindTypeByName("ezIntegerStruct");
  ezReflectedTypeHandle intHandle;
  const ezRTTI* pRttiFloat = ezRTTI::FindTypeByName("ezFloatStruct");
  ezReflectedTypeHandle floatHandle;
  const ezRTTI* pRttiPOD = ezRTTI::FindTypeByName("ezPODClass");
  ezReflectedTypeHandle podHandle;
  const ezRTTI* pRttiMath = ezRTTI::FindTypeByName("ezMathClass");
  ezReflectedTypeHandle mathHandle;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RegisterType")
  {
    EZ_TEST_INT(ezReflectedTypeManager::GetTypeCount(), 0);
    baseHandle = RegisterTypeViaRtti(pRttiBase);
    intHandle = RegisterTypeViaRtti(pRttiInt);
    floatHandle = RegisterTypeViaRtti(pRttiFloat);
    podHandle = RegisterTypeViaRtti(pRttiPOD);
    mathHandle = RegisterTypeViaRtti(pRttiMath);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezReflectedTypeDirectAccessor")
  {
    ezIntegerStruct intStruct;
    ezReflectedTypeDirectAccessor intAccessor(&intStruct, pRttiInt);
    EZ_TEST_BOOL(intAccessor.GetReflectedTypeHandle() == intHandle);
    EZ_TEST_INT(AccessorPropertiesTest(intAccessor), 8);

    ezFloatStruct floatStruct;
    ezReflectedTypeDirectAccessor floatAccessor(&floatStruct, pRttiFloat);
    EZ_TEST_BOOL(floatAccessor.GetReflectedTypeHandle() == floatHandle);
    EZ_TEST_INT(AccessorPropertiesTest(floatAccessor), 3);

    ezPODClass podClass;
    ezReflectedTypeDirectAccessor podAccessor(&podClass);
    EZ_TEST_BOOL(podAccessor.GetReflectedTypeHandle() == podHandle);
    EZ_TEST_INT(AccessorPropertiesTest(podAccessor), 14);

    ezMathClass mathClass;
    ezReflectedTypeDirectAccessor mathAccessor(&mathClass);
    EZ_TEST_BOOL(mathAccessor.GetReflectedTypeHandle() == mathHandle);
    EZ_TEST_INT(AccessorPropertiesTest(mathAccessor), 20);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezReflectedTypeStorageAccessor")
  {
    ezReflectedTypeStorageAccessor intAccessor(intHandle);
    EZ_TEST_INT(AccessorPropertiesTest(intAccessor), 8);

    ezReflectedTypeStorageAccessor floatAccessor(floatHandle);
    EZ_TEST_INT(AccessorPropertiesTest(floatAccessor), 3);

    ezReflectedTypeStorageAccessor podAccessor(podHandle);
    EZ_TEST_INT(AccessorPropertiesTest(podAccessor), 14);

    ezReflectedTypeStorageAccessor mathAccessor(mathHandle);
    EZ_TEST_INT(AccessorPropertiesTest(mathAccessor), 20);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "UnregisterType")
  {
    EZ_TEST_INT(ezReflectedTypeManager::GetTypeCount(), 5);
    ezReflectedTypeManager::UnregisterType(mathHandle);
    ezReflectedTypeManager::UnregisterType(podHandle);
    ezReflectedTypeManager::UnregisterType(floatHandle);
    ezReflectedTypeManager::UnregisterType(intHandle);
    ezReflectedTypeManager::UnregisterType(baseHandle);
    EZ_TEST_INT(ezReflectedTypeManager::GetTypeCount(), 0);
  }
}


struct InnerStruct
{
public:
  float m_fP1;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, InnerStruct);

EZ_BEGIN_STATIC_REFLECTED_TYPE(InnerStruct, ezNoBase, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("IP1", m_fP1),
  EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();


class OuterClass : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(OuterClass);

public:
  InnerStruct m_Inner1;
  float m_fP1;
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(OuterClass, ezReflectedClass, 1, ezRTTIDefaultAllocator<OuterClass>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Inner", m_Inner1),
    EZ_MEMBER_PROPERTY("OP1", m_fP1),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();


EZ_CREATE_SIMPLE_TEST(Reflection, ReflectedTypeReloading)
{
  const ezRTTI* pRttiInner = ezRTTI::FindTypeByName("InnerStruct");
  ezReflectedTypeHandle InnerHandle;
  ezReflectedTypeDescriptor descInner;

  const ezRTTI* pRttiOuter = ezRTTI::FindTypeByName("OuterClass");
  ezReflectedTypeHandle OuterHandle;
  ezReflectedTypeDescriptor descOuter;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RegisterType")
  {
    const ezRTTI* pRttiBase = ezRTTI::FindTypeByName("ezReflectedClass");
    RegisterTypeViaRtti(pRttiBase);

    EZ_TEST_BOOL(pRttiInner != nullptr);
    ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiInner, descInner);
    InnerHandle = ezReflectedTypeManager::RegisterType(descInner);
    EZ_TEST_BOOL(!InnerHandle.IsInvalidated());

    EZ_TEST_BOOL(pRttiOuter != nullptr);
    ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiOuter, descOuter);
    OuterHandle = ezReflectedTypeManager::RegisterType(descOuter);
    EZ_TEST_BOOL(!OuterHandle.IsInvalidated());
  }

  ezReflectedTypeStorageAccessor innerAccessor(InnerHandle);
  ezReflectedTypeStorageAccessor outerAccessor(OuterHandle);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetValues")
  {
    // Just set a few values to make sure they don't get messed up by the following operations.
    ezPropertyPath path = ezToolsReflectionUtils::CreatePropertyPath("IP1");
    EZ_TEST_BOOL(innerAccessor.SetValue(path, 1.4f));

    path = ezToolsReflectionUtils::CreatePropertyPath("OP1");
    EZ_TEST_BOOL(outerAccessor.SetValue(path, 0.9f));
    path = ezToolsReflectionUtils::CreatePropertyPath("Inner", "IP1");
    EZ_TEST_BOOL(outerAccessor.SetValue(path, 1.4f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddProperty")
  {
    // Say we reload the engine and the InnerStruct now has a second property: IP2.
    descInner.m_Properties.PushBack(ezReflectedPropertyDescriptor("IP2", ezVariant::Type::Vector4, PropertyFlags::IsPOD));
    ezReflectedTypeHandle NewInnerHandle = ezReflectedTypeManager::RegisterType(descInner);
    EZ_TEST_BOOL(NewInnerHandle == InnerHandle);

    // Check that the new property is present.
    ezPropertyPath path = ezToolsReflectionUtils::CreatePropertyPath("IP2");
    AccessorPropertyTest(innerAccessor, path, ezVariant::Type::Vector4);

    path = ezToolsReflectionUtils::CreatePropertyPath("Inner", "IP2");
    AccessorPropertyTest(outerAccessor, path, ezVariant::Type::Vector4);

    // Test that the old properties are still valid.
    path = ezToolsReflectionUtils::CreatePropertyPath("IP1");
    EZ_TEST_BOOL(innerAccessor.GetValue(path) == 1.4f);

    path = ezToolsReflectionUtils::CreatePropertyPath("OP1");
    EZ_TEST_BOOL(outerAccessor.GetValue(path) == 0.9f);
    path = ezToolsReflectionUtils::CreatePropertyPath("Inner", "IP1");
    EZ_TEST_BOOL(outerAccessor.GetValue(path) == 1.4f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ChangeProperty")
  {
    // Out original inner float now is a Int32!
    descInner.m_Properties[0].m_Type = ezVariant::Type::Int32;
    ezReflectedTypeHandle NewInnerHandle = ezReflectedTypeManager::RegisterType(descInner);
    EZ_TEST_BOOL(NewInnerHandle == InnerHandle);

    // Test if the previous value was converted correctly to its new type.
    ezPropertyPath path = ezToolsReflectionUtils::CreatePropertyPath("IP1");
    ezVariant innerValue = innerAccessor.GetValue(path);
    EZ_TEST_BOOL(innerValue.IsValid());
    EZ_TEST_BOOL(innerValue.GetType() == ezVariant::Type::Int32);
    EZ_TEST_INT(innerValue.Get<ezInt32>(), 1);

    path = ezToolsReflectionUtils::CreatePropertyPath("Inner", "IP1");
    ezVariant outerValue = outerAccessor.GetValue(path);
    EZ_TEST_BOOL(outerValue.IsValid());
    EZ_TEST_BOOL(outerValue.GetType() == ezVariant::Type::Int32);
    EZ_TEST_INT(outerValue.Get<ezInt32>(), 1);

    // Test that the old properties are still valid.
    path = ezToolsReflectionUtils::CreatePropertyPath("OP1");
    EZ_TEST_BOOL(outerAccessor.GetValue(path) == 0.9f);

    path = ezToolsReflectionUtils::CreatePropertyPath("IP2");
    AccessorPropertyTest(innerAccessor, path, ezVariant::Type::Vector4);

    path = ezToolsReflectionUtils::CreatePropertyPath("Inner", "IP2");
    AccessorPropertyTest(outerAccessor, path, ezVariant::Type::Vector4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "DeleteProperty")
  {
    // Lets now delete the original inner property IP1.
    descInner.m_Properties.RemoveAt(0);
    ezReflectedTypeHandle NewInnerHandle = ezReflectedTypeManager::RegisterType(descInner);
    EZ_TEST_BOOL(NewInnerHandle == InnerHandle);

    // Check that IP1 is really gone.
    ezPropertyPath path = ezToolsReflectionUtils::CreatePropertyPath("IP1");
    EZ_TEST_BOOL(!innerAccessor.GetValue(path).IsValid());

    path = ezToolsReflectionUtils::CreatePropertyPath("Inner", "IP1");
    EZ_TEST_BOOL(!outerAccessor.GetValue(path).IsValid());

    // Test that the old properties are still valid.
    path = ezToolsReflectionUtils::CreatePropertyPath("OP1");
    EZ_TEST_BOOL(outerAccessor.GetValue(path) == 0.9f);

    path = ezToolsReflectionUtils::CreatePropertyPath("IP2");
    AccessorPropertyTest(innerAccessor, path, ezVariant::Type::Vector4);

    path = ezToolsReflectionUtils::CreatePropertyPath("Inner", "IP2");
    AccessorPropertyTest(outerAccessor, path, ezVariant::Type::Vector4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RevertProperties")
  {
    // Reset all classes to their initial state.
    ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiInner, descInner);
    ezReflectedTypeManager::RegisterType(descInner);

    ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiOuter, descOuter);
    ezReflectedTypeManager::RegisterType(descOuter);

    // Test that the old properties are back again.
    ezPropertyPath path = ezToolsReflectionUtils::CreatePropertyPath("IP1");
    ezVariant innerValue = innerAccessor.GetValue(path);
    EZ_TEST_BOOL(innerValue.IsValid());
    EZ_TEST_BOOL(innerValue.GetType() == ezVariant::Type::Float);
    EZ_TEST_FLOAT(innerValue.Get<float>(), 1.0f, 0.0f);

    path = ezToolsReflectionUtils::CreatePropertyPath("Inner", "IP1");
    ezVariant outerValue = outerAccessor.GetValue(path);
    EZ_TEST_BOOL(outerValue.IsValid());
    EZ_TEST_BOOL(outerValue.GetType() == ezVariant::Type::Float);
    EZ_TEST_FLOAT(outerValue.Get<float>(), 1.0f, 0.0f);

    path = ezToolsReflectionUtils::CreatePropertyPath("OP1");
    EZ_TEST_BOOL(outerAccessor.GetValue(path) == 0.9f);
  }
}