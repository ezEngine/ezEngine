#pragma once

#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VarianceTypes.h>

struct ezVariantTestStruct
{
public:
  ezVariant m_Variant;
  ezVariantArray m_VariantArray;
  ezVariantDictionary m_VariantDictionary;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezVariantTestStruct);

struct ezIntegerStruct
{
public:
  ezIntegerStruct()
  {
    m_iInt8 = 1;
    m_uiUInt8 = 1;
    m_iInt16 = 1;
    m_iUInt16 = 1;
    m_iInt32 = 1;
    m_uiUInt32 = 1;
    m_iInt64 = 1;
    m_iUInt64 = 1;
  }

  void SetInt8(ezInt8 i) { m_iInt8 = i; }
  ezInt8 GetInt8() const { return m_iInt8; }
  void SetUInt8(ezUInt8 i) { m_uiUInt8 = i; }
  ezUInt8 GetUInt8() const { return m_uiUInt8; }
  void SetInt32(ezInt32 i) { m_iInt32 = i; }
  ezInt32 GetInt32() const { return m_iInt32; }
  void SetUInt32(ezUInt32 i) { m_uiUInt32 = i; }
  ezUInt32 GetUInt32() const { return m_uiUInt32; }

  ezInt16 m_iInt16;
  ezUInt16 m_iUInt16;
  ezInt64 m_iInt64;
  ezUInt64 m_iUInt64;

private:
  ezInt8 m_iInt8;
  ezUInt8 m_uiUInt8;
  ezInt32 m_iInt32;
  ezUInt32 m_uiUInt32;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezIntegerStruct);


struct ezFloatStruct
{
public:
  ezFloatStruct()
  {
    m_fFloat = 1.0f;
    m_fDouble = 1.0;
    m_Time = ezTime::MakeFromSeconds(1.0);
    m_Angle = ezAngle::MakeFromDegree(45.0f);
  }

  void SetFloat(float f) { m_fFloat = f; }
  float GetFloat() const { return m_fFloat; }
  void SetDouble(double d) { m_fDouble = d; }
  double GetDouble() const { return m_fDouble; }
  void SetTime(ezTime t) { m_Time = t; }
  ezTime GetTime() const { return m_Time; }
  ezAngle GetAngle() const { return m_Angle; }
  void SetAngle(ezAngle t) { m_Angle = t; }

private:
  float m_fFloat;
  double m_fDouble;
  ezTime m_Time;
  ezAngle m_Angle;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezFloatStruct);


class ezPODClass : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPODClass, ezReflectedClass);

public:
  ezPODClass()
  {
    m_bBool = true;
    m_Color = ezColor(1.0f, 0.0f, 0.0f, 0.0f);
    m_Color2 = ezColorGammaUB(255, 10, 1);
    m_sCharPtr = "Test";
    m_sString = "Test";
    m_sStringView = "Test";
    m_Buffer.PushBack(0xFF);
    m_Buffer.PushBack(0x0);
    m_Buffer.PushBack(0xCD);
    m_VarianceAngle = ezVarianceTypeAngle(ezAngle::MakeFromDegree(90.0f), 0.1f);
  }

  ezIntegerStruct m_IntegerStruct;
  ezFloatStruct m_FloatStruct;

  bool GetBool() const { return m_bBool; }
  void SetBool(bool b) { m_bBool = b; }

  ezColor GetColor() const { return m_Color; }
  void SetColor(ezColor c) { m_Color = c; }

  const char* GetCharPtr() const { return m_sCharPtr.GetData(); }
  void SetCharPtr(const char* szSz) { m_sCharPtr = szSz; }

  const ezString& GetString() const { return m_sString; }
  void SetString(const ezString& sStr) { m_sString = sStr; }

  ezStringView GetStringView() const { return m_sStringView.GetView(); }
  void SetStringView(ezStringView sStrView) { m_sStringView = sStrView; }

  const ezDataBuffer& GetBuffer() const { return m_Buffer; }
  void SetBuffer(const ezDataBuffer& data) { m_Buffer = data; }

  ezVarianceTypeAngle GetCustom() const { return m_VarianceAngle; }
  void SetCustom(ezVarianceTypeAngle value) { m_VarianceAngle = value; }

private:
  bool m_bBool;
  ezColor m_Color;
  ezColorGammaUB m_Color2;
  ezString m_sCharPtr;
  ezString m_sString;
  ezString m_sStringView;
  ezDataBuffer m_Buffer;
  ezVarianceTypeAngle m_VarianceAngle;
};


class ezMathClass : public ezPODClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMathClass, ezPODClass);

public:
  ezMathClass()
  {
    m_vVec2 = ezVec2(1.0f, 1.0f);
    m_vVec3 = ezVec3(1.0f, 1.0f, 1.0f);
    m_vVec4 = ezVec4(1.0f, 1.0f, 1.0f, 1.0f);
    m_Vec2I = ezVec2I32(1, 1);
    m_Vec3I = ezVec3I32(1, 1, 1);
    m_Vec4I = ezVec4I32(1, 1, 1, 1);
    m_qQuat = ezQuat(1.0f, 1.0f, 1.0f, 1.0f);
    m_mMat3.SetZero();
    m_mMat4.SetZero();
  }

  void SetVec2(ezVec2 v) { m_vVec2 = v; }
  ezVec2 GetVec2() const { return m_vVec2; }
  void SetVec3(ezVec3 v) { m_vVec3 = v; }
  ezVec3 GetVec3() const { return m_vVec3; }
  void SetVec4(ezVec4 v) { m_vVec4 = v; }
  ezVec4 GetVec4() const { return m_vVec4; }
  void SetQuat(ezQuat q) { m_qQuat = q; }
  ezQuat GetQuat() const { return m_qQuat; }
  void SetMat3(ezMat3 m) { m_mMat3 = m; }
  ezMat3 GetMat3() const { return m_mMat3; }
  void SetMat4(ezMat4 m) { m_mMat4 = m; }
  ezMat4 GetMat4() const { return m_mMat4; }

  ezVec2I32 m_Vec2I;
  ezVec3I32 m_Vec3I;
  ezVec4I32 m_Vec4I;

private:
  ezVec2 m_vVec2;
  ezVec3 m_vVec3;
  ezVec4 m_vVec4;
  ezQuat m_qQuat;
  ezMat3 m_mMat3;
  ezMat4 m_mMat4;
};


struct ezExampleEnum
{
  using StorageType = ezInt8;
  enum Enum
  {
    Value1 = 0,      // normal value
    Value2 = -2,     // normal value
    Value3 = 4,      // normal value
    Default = Value1 // Default initialization value (required)
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezExampleEnum);


struct ezExampleBitflags
{
  using StorageType = ezUInt64;
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


class ezEnumerationsClass : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEnumerationsClass, ezReflectedClass);

public:
  ezEnumerationsClass()
  {
    m_EnumClass = ezExampleEnum::Value2;
    m_BitflagsClass = ezExampleBitflags::Value2;
  }

  void SetEnum(ezExampleEnum::Enum e) { m_EnumClass = e; }
  ezExampleEnum::Enum GetEnum() const { return m_EnumClass; }
  void SetBitflags(ezBitflags<ezExampleBitflags> e) { m_BitflagsClass = e; }
  ezBitflags<ezExampleBitflags> GetBitflags() const { return m_BitflagsClass; }

private:
  ezEnum<ezExampleEnum> m_EnumClass;
  ezBitflags<ezExampleBitflags> m_BitflagsClass;
};


struct InnerStruct
{
  EZ_DECLARE_POD_TYPE();

public:
  float m_fP1;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, InnerStruct);


class OuterClass : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(OuterClass, ezReflectedClass);

public:
  InnerStruct m_Inner1;
  float m_fP1;
};

class ExtendedOuterClass : public OuterClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ExtendedOuterClass, OuterClass);

public:
  ezString m_more;
};

class ezObjectTest : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezObjectTest, ezReflectedClass);

public:
  ezObjectTest() = default;
  ~ezObjectTest()
  {
    for (OuterClass* pTest : m_ClassPtrArray)
    {
      ezGetStaticRTTI<OuterClass>()->GetAllocator()->Deallocate(pTest);
    }
    for (ezObjectTest* pTest : m_SubObjectSet)
    {
      ezGetStaticRTTI<ezObjectTest>()->GetAllocator()->Deallocate(pTest);
    }
    for (auto it = m_ClassPtrMap.GetIterator(); it.IsValid(); ++it)
    {
      ezGetStaticRTTI<OuterClass>()->GetAllocator()->Deallocate(it.Value());
    }
  }

  ezArrayPtr<const ezString> GetStandardTypeSet() const;
  void StandardTypeSetInsert(const ezString& value);
  void StandardTypeSetRemove(const ezString& value);

  OuterClass m_MemberClass;

  ezDynamicArray<double> m_StandardTypeArray;
  ezDynamicArray<OuterClass> m_ClassArray;
  ezDeque<OuterClass*> m_ClassPtrArray;

  ezDynamicArray<ezString> m_StandardTypeSet;
  ezSet<ezObjectTest*> m_SubObjectSet;

  ezMap<ezString, double> m_StandardTypeMap;
  ezHashTable<ezString, OuterClass> m_ClassMap;
  ezMap<ezString, OuterClass*> m_ClassPtrMap;
};


class ezMirrorTest : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMirrorTest, ezReflectedClass);

public:
  ezMirrorTest() = default;

  ezMathClass m_math;
  ezObjectTest m_object;
};
