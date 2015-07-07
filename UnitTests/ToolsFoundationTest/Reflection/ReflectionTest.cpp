#include <PCH.h>
#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
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


struct ezExampleEnum
{
  typedef ezInt8 StorageType;
  enum Enum
  {
    Value1 = 0,          // normal value
    Value2 = -2,         // normal value
    Value3 = 4,          // normal value
    Default = Value1     // Default initialization value (required)
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezExampleEnum);

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezExampleEnum, 1)
  EZ_ENUM_CONSTANTS(ezExampleEnum::Value1, ezExampleEnum::Value2) 
  EZ_ENUM_CONSTANT(ezExampleEnum::Value3),
  EZ_END_STATIC_REFLECTED_ENUM();


struct ezExampleBitflags
{
  typedef ezUInt64 StorageType;
  enum Enum : ezUInt64
  {
    Value1 = EZ_BIT(0),  // normal value
    Value2 = EZ_BIT(31), // normal value
    Value3 = EZ_BIT(63), // normal value
    Default = Value1     // Default initialization value (required)
  };

  struct Bits
  {
    StorageType Value1 : 1;
    StorageType Padding : 30;
    StorageType Value2 : 1;
    StorageType Padding2 : 31;
    StorageType Value3 : 1;
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezExampleBitflags);

EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezExampleBitflags, 1)
  EZ_BITFLAGS_CONSTANTS(ezExampleBitflags::Value1, ezExampleBitflags::Value2) 
  EZ_BITFLAGS_CONSTANT(ezExampleBitflags::Value3),
  EZ_END_STATIC_REFLECTED_BITFLAGS();


class ezEnumerationsClass : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEnumerationsClass);

public:
  ezEnumerationsClass()
  {
    m_enumClass = ezExampleEnum::Value2;
    m_bitflagsClass = ezExampleBitflags::Value2;
  }

  void SetEnum(ezExampleEnum::Enum e) { m_enumClass = e; }
  ezExampleEnum::Enum GetEnum() const { return m_enumClass; }
  void SetBitflags(ezBitflags<ezExampleBitflags> e) { m_bitflagsClass = e; }
  ezBitflags<ezExampleBitflags> GetBitflags() const { return m_bitflagsClass; }

private:
  ezEnum<ezExampleEnum> m_enumClass;
  ezBitflags<ezExampleBitflags> m_bitflagsClass;
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEnumerationsClass, ezReflectedClass, 1, ezRTTIDefaultAllocator<ezEnumerationsClass>);
EZ_BEGIN_PROPERTIES
  EZ_ENUM_ACCESSOR_PROPERTY("Enum", ezExampleEnum, GetEnum, SetEnum),
  EZ_BITFLAGS_ACCESSOR_PROPERTY("Bitflags", ezExampleBitflags, GetBitflags, SetBitflags),
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Enumeration Properties")
  {
    ezEnumerationsClass enumClass;
    ezRTTI* pRttiEnum = ezRTTI::FindTypeByName("ezEnumerationsClass");
    EZ_TEST_BOOL(pRttiEnum != nullptr);

    VariantToPropertyTest(&enumClass, pRttiEnum, "Enum", ezVariant::Type::Int64);
    EZ_TEST_BOOL(enumClass.GetEnum() == ezExampleEnum::Value1);
    VariantToPropertyTest(&enumClass, pRttiEnum, "Bitflags", ezVariant::Type::Int64);
    EZ_TEST_BOOL(enumClass.GetBitflags() == 0);
  }
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

ezUInt32 AccessorPropertiesTest(ezIReflectedTypeAccessor& accessor, const ezRTTI* pType, ezPropertyPath& path)
{
  ezUInt32 uiPropertiesSet = 0;
  EZ_TEST_BOOL(pType != nullptr);

  // Call for base class
  if (pType->GetParentType() != nullptr)
  {
    uiPropertiesSet += AccessorPropertiesTest(accessor, pType->GetParentType(), path);
  }

  // Test properties
  ezUInt32 uiPropCount = pType->GetProperties().GetCount();
  for (ezUInt32 i = 0; i < uiPropCount; ++i)
  {
    ezAbstractProperty* pProp = pType->GetProperties()[i];
    // Build path to property
    ezPropertyPath propPath = path;
    propPath.PushBack(pProp->GetPropertyName());

    switch (pProp->GetCategory())
    {
    case ezPropertyCategory::Member:
      {
        ezAbstractMemberProperty* pProp3 = static_cast<ezAbstractMemberProperty*>(pProp);
        if (pProp->GetFlags().IsSet(ezPropertyFlags::IsEnum))
        {
          AccessorPropertyTest(accessor, propPath, ezVariant::Type::Int64);
          uiPropertiesSet++;
        }
        else if (pProp->GetFlags().IsSet(ezPropertyFlags::Bitflags))
        {
          AccessorPropertyTest(accessor, propPath, ezVariant::Type::Int64);
          uiPropertiesSet++;
        }
        else if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
        {
          AccessorPropertyTest(accessor, propPath, pProp3->GetSpecificType()->GetVariantType());
          uiPropertiesSet++;
        }
        else
        {
          // Recurs into sub-classes
          uiPropertiesSet += AccessorPropertiesTest(accessor, pProp3->GetSpecificType(), propPath);
        }
      }
      break;
    case ezPropertyCategory::Array:
      {
        ezAbstractArrayProperty* pProp3 = static_cast<ezAbstractArrayProperty*>(pProp);
        // TODO
      }
      break;
    }
  }
  return uiPropertiesSet;
}

ezUInt32 AccessorPropertiesTest(ezIReflectedTypeAccessor& accessor)
{
  const ezRTTI* handle = accessor.GetType();
  ezPropertyPath path = ezPropertyPath();
  return AccessorPropertiesTest(accessor, handle, path);
}

static ezUInt32 GetTypeCount()
{
  ezUInt32 uiCount = 0;
  ezRTTI* pType = ezRTTI::GetFirstInstance();
  while (pType != nullptr)
  {
    uiCount++;
    pType = pType->GetNextInstance();
  }
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
  const ezRTTI* pRttiBase = RegisterType("ezReflectedClass");
  const ezRTTI* pRttiEnumBase = RegisterType("ezEnumBase");
  const ezRTTI* pRttiBitflagsBase = RegisterType("ezBitflagsBase");

  const ezRTTI* pRttiInt = RegisterType("ezIntegerStruct");
  const ezRTTI* pRttiFloat = RegisterType("ezFloatStruct");
  const ezRTTI* pRttiPOD = RegisterType("ezPODClass");
  const ezRTTI* pRttiMath = RegisterType("ezMathClass");
  const ezRTTI* pRttiEnum = RegisterType("ezExampleEnum");
  const ezRTTI* pRttiFlags = RegisterType("ezExampleBitflags");
  const ezRTTI* pRttiEnumerations = RegisterType("ezEnumerationsClass");

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezReflectedTypeDirectAccessor")
  {
    ezIntegerStruct intStruct;
    ezReflectedTypeDirectAccessor intAccessor(&intStruct, pRttiInt);
    EZ_TEST_BOOL(intAccessor.GetType() == pRttiInt);
    EZ_TEST_INT(AccessorPropertiesTest(intAccessor), 8);

    ezFloatStruct floatStruct;
    ezReflectedTypeDirectAccessor floatAccessor(&floatStruct, pRttiFloat);
    EZ_TEST_BOOL(floatAccessor.GetType() == pRttiFloat);
    EZ_TEST_INT(AccessorPropertiesTest(floatAccessor), 3);

    ezPODClass podClass;
    ezReflectedTypeDirectAccessor podAccessor(&podClass);
    EZ_TEST_BOOL(podAccessor.GetType() == pRttiPOD);
    EZ_TEST_INT(AccessorPropertiesTest(podAccessor), 14);

    ezMathClass mathClass;
    ezReflectedTypeDirectAccessor mathAccessor(&mathClass);
    EZ_TEST_BOOL(mathAccessor.GetType() == pRttiMath);
    EZ_TEST_INT(AccessorPropertiesTest(mathAccessor), 20);

    ezEnumerationsClass enumerationsClass;
    ezReflectedTypeDirectAccessor enumerationsAccessor(&enumerationsClass);
    EZ_TEST_BOOL(enumerationsAccessor.GetType() == pRttiEnumerations);
    EZ_TEST_INT(AccessorPropertiesTest(enumerationsAccessor), 2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezReflectedTypeStorageAccessor")
  {
    ezReflectedTypeStorageAccessor intAccessor(pRttiInt);
    EZ_TEST_INT(AccessorPropertiesTest(intAccessor), 8);

    ezReflectedTypeStorageAccessor floatAccessor(pRttiFloat);
    EZ_TEST_INT(AccessorPropertiesTest(floatAccessor), 3);

    ezReflectedTypeStorageAccessor podAccessor(pRttiPOD);
    EZ_TEST_INT(AccessorPropertiesTest(podAccessor), 14);

    ezReflectedTypeStorageAccessor mathAccessor(pRttiMath);
    EZ_TEST_INT(AccessorPropertiesTest(mathAccessor), 20);

    ezReflectedTypeStorageAccessor enumerationsAccessor(pRttiEnumerations);
    EZ_TEST_INT(AccessorPropertiesTest(enumerationsAccessor), 2);
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
  const ezRTTI* pRttiInnerP = nullptr;
  ezReflectedTypeDescriptor descInner;

  const ezRTTI* pRttiOuter = ezRTTI::FindTypeByName("OuterClass");
  const ezRTTI* pRttiOuterP = nullptr;
  ezReflectedTypeDescriptor descOuter;

  ezUInt32 uiRegisteredBaseTypes = GetTypeCount();
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RegisterType")
  {
    const ezRTTI* pRttiBase = ezRTTI::FindTypeByName("ezReflectedClass");

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
    ezReflectedTypeStorageAccessor innerAccessor(pRttiInnerP);
    ezReflectedTypeStorageAccessor outerAccessor(pRttiOuterP);

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
      descInner.m_Properties.PushBack(ezReflectedPropertyDescriptor(ezPropertyCategory::Member, "IP2", "ezVec4", ezVariant::Type::Vector4, ezBitflags<ezPropertyFlags>(ezPropertyFlags::StandardType)));
      const ezRTTI* NewInnerHandle = ezPhantomRttiManager::RegisterType(descInner);
      EZ_TEST_BOOL(NewInnerHandle == pRttiInnerP);

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
      descInner.m_Properties[0].m_sType = "ezInt32";
      const ezRTTI* NewInnerHandle = ezPhantomRttiManager::RegisterType(descInner);
      EZ_TEST_BOOL(NewInnerHandle == pRttiInnerP);

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
      const ezRTTI* NewInnerHandle = ezPhantomRttiManager::RegisterType(descInner);
      EZ_TEST_BOOL(NewInnerHandle == pRttiInnerP);

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
      descInner.m_sTypeName = "InnerStructP";
      ezPhantomRttiManager::RegisterType(descInner);

      ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiOuter, descOuter);
      descInner.m_sTypeName = "OuterStructP";
      descOuter.m_Properties[0].m_sType = "InnerStructP";
      ezPhantomRttiManager::RegisterType(descOuter);

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "UnregisterType")
  {
    EZ_TEST_INT(GetTypeCount(), uiRegisteredBaseTypes + 2);
    ezPhantomRttiManager::UnregisterType(pRttiOuterP);
    ezPhantomRttiManager::UnregisterType(pRttiInnerP);
    EZ_TEST_INT(GetTypeCount(), uiRegisteredBaseTypes);
  }
}