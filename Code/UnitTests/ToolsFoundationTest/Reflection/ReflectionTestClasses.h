#pragma once

#include <Foundation/Reflection/Reflection.h>

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


struct ezFloatStruct
{
public:
  ezFloatStruct()
  {
    m_fFloat = 1.0f;
    m_fDouble = 1.0;
    m_Time = ezTime::Seconds(1.0);
    m_Angle = ezAngle::Degree(45.0f);
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
    m_sString = "Test";
    m_Buffer.PushBack(0xFF);
    m_Buffer.PushBack(0x0);
    m_Buffer.PushBack(0xCD);
  }

  ezIntegerStruct m_IntegerStruct;
  ezFloatStruct m_FloatStruct;

  void SetBool(bool b) { m_bBool = b; }
  bool GetBool() const { return m_bBool; }
  void SetColor(ezColor c) { m_Color = c; }
  ezColor GetColor() const { return m_Color; }
  const char* GetString() const { return m_sString.GetData(); }
  void SetString(const char* sz) { m_sString = sz; }

  const ezDataBuffer& GetBuffer() const { return m_Buffer; }
  void SetBuffer(const ezDataBuffer& data) { m_Buffer = data; }

private:
  bool m_bBool;
  ezColor m_Color;
  ezString m_sString;
  ezDataBuffer m_Buffer;
};


class ezMathClass : public ezPODClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMathClass, ezPODClass);

public:
  ezMathClass()
  {
    m_Vec2 = ezVec2(1.0f, 1.0f);
    m_Vec3 = ezVec3(1.0f, 1.0f, 1.0f);
    m_Vec4 = ezVec4(1.0f, 1.0f, 1.0f, 1.0f);
    m_Vec2I = ezVec2I32(1, 1);
    m_Vec3I = ezVec3I32(1, 1, 1);
    m_Vec4I = ezVec4I32(1, 1, 1, 1);
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

  ezVec2I32 m_Vec2I;
  ezVec3I32 m_Vec3I;
  ezVec4I32 m_Vec4I;

private:
  ezVec2 m_Vec2;
  ezVec3 m_Vec3;
  ezVec4 m_Vec4;
  ezQuat m_Quat;
  ezMat3 m_Mat3;
  ezMat4 m_Mat4;
};


struct ezExampleEnum
{
  typedef ezInt8 StorageType;
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


class ezEnumerationsClass : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEnumerationsClass, ezReflectedClass);

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
  ezObjectTest() {}
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
  ezMirrorTest() {}

  ezMathClass m_math;
  ezObjectTest m_object;
};
