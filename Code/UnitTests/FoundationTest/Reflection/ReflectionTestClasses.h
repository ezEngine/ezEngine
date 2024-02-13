#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/VarianceTypes.h>

struct ezExampleEnum
{
  using StorageType = ezInt8;
  enum Enum
  {
    Value1 = 1,      // normal value
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

EZ_DECLARE_FLAGS_OPERATORS(ezExampleBitflags);

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezExampleBitflags);


class ezAbstractTestClass : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAbstractTestClass, ezReflectedClass);

  virtual void AbstractFunction() = 0;
};


struct ezAbstractTestStruct
{
  virtual void AbstractFunction() = 0;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezAbstractTestStruct);


struct ezTestStruct
{
  EZ_ALLOW_PRIVATE_PROPERTIES(ezTestStruct);

public:
  static ezDataBuffer GetDefaultDataBuffer()
  {
    ezDataBuffer data;
    data.PushBack(255);
    data.PushBack(0);
    data.PushBack(127);
    return data;
  }

  ezTestStruct()
  {
    m_fFloat1 = 1.1f;
    m_iInt2 = 2;
    m_vProperty3.Set(3, 4, 5);
    m_UInt8 = 6;
    m_variant = "Test";
    m_Angle = ezAngle::MakeFromDegree(0.5);
    m_DataBuffer = GetDefaultDataBuffer();
    m_vVec3I = ezVec3I32(1, 2, 3);
    m_VarianceAngle.m_fVariance = 0.5f;
    m_VarianceAngle.m_Value = ezAngle::MakeFromDegree(90.0f);
  }



  bool operator==(const ezTestStruct& rhs) const
  {
    return m_fFloat1 == rhs.m_fFloat1 && m_UInt8 == rhs.m_UInt8 && m_variant == rhs.m_variant && m_iInt2 == rhs.m_iInt2 && m_vProperty3 == rhs.m_vProperty3 && m_Angle == rhs.m_Angle && m_DataBuffer == rhs.m_DataBuffer && m_vVec3I == rhs.m_vVec3I && m_VarianceAngle == rhs.m_VarianceAngle;
  }

  float m_fFloat1;
  ezUInt8 m_UInt8;
  ezVariant m_variant;
  ezAngle m_Angle;
  ezDataBuffer m_DataBuffer;
  ezVec3I32 m_vVec3I;
  ezVarianceTypeAngle m_VarianceAngle;

private:
  void SetInt(ezInt32 i) { m_iInt2 = i; }
  ezInt32 GetInt() const { return m_iInt2; }

  ezInt32 m_iInt2;
  ezVec3 m_vProperty3;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTestStruct);


struct ezTestStruct3
{
  EZ_ALLOW_PRIVATE_PROPERTIES(ezTestStruct3);

public:
  ezTestStruct3()
  {
    m_fFloat1 = 1.1f;
    m_UInt8 = 6;
    m_iInt32 = 2;
  }
  ezTestStruct3(double a, ezInt16 b)
  {
    m_fFloat1 = a;
    m_UInt8 = b;
    m_iInt32 = 32;
  }

  bool operator==(const ezTestStruct3& rhs) const { return m_fFloat1 == rhs.m_fFloat1 && m_iInt32 == rhs.m_iInt32 && m_UInt8 == rhs.m_UInt8; }

  bool operator!=(const ezTestStruct3& rhs) const { return !(*this == rhs); }

  double m_fFloat1;
  ezInt16 m_UInt8;

  ezUInt32 GetIntPublic() const { return m_iInt32; }

private:
  void SetInt(ezUInt32 i) { m_iInt32 = i; }
  ezUInt32 GetInt() const { return m_iInt32; }

  ezInt32 m_iInt32;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTestStruct3);

struct ezTypedObjectStruct
{
  EZ_ALLOW_PRIVATE_PROPERTIES(ezTypedObjectStruct);

public:
  ezTypedObjectStruct()
  {
    m_fFloat1 = 1.1f;
    m_UInt8 = 6;
    m_iInt32 = 2;
  }
  ezTypedObjectStruct(double a, ezInt16 b)
  {
    m_fFloat1 = a;
    m_UInt8 = b;
    m_iInt32 = 32;
  }

  double m_fFloat1;
  ezInt16 m_UInt8;
  ezInt32 m_iInt32;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTypedObjectStruct);
EZ_DECLARE_CUSTOM_VARIANT_TYPE(ezTypedObjectStruct);

class ezTestClass1 : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestClass1, ezReflectedClass);

public:
  ezTestClass1()
  {
    m_MyVector.Set(3, 4, 5);

    m_Struct.m_fFloat1 = 33.3f;

    m_Color = ezColor::CornflowerBlue; // The Original!
  }

  ezTestClass1(const ezColor& c, const ezTestStruct& s)
  {
    m_Color = c;
    m_Struct = s;
    m_MyVector.Set(1, 2, 3);
  }

  bool operator==(const ezTestClass1& rhs) const { return m_Struct == rhs.m_Struct && m_MyVector == rhs.m_MyVector && m_Color == rhs.m_Color; }

  ezVec3 GetVector() const { return m_MyVector; }

  ezTestStruct m_Struct;
  ezVec3 m_MyVector;
  ezColor m_Color;
};


class ezTestClass2 : public ezTestClass1
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestClass2, ezTestClass1);

public:
  ezTestClass2() { m_sCharPtr = "AAA"; m_sString = "BBB"; m_sStringView = "CCC"; }

  bool operator==(const ezTestClass2& rhs) const { return m_Time == rhs.m_Time && m_enumClass == rhs.m_enumClass && m_bitflagsClass == rhs.m_bitflagsClass && m_array == rhs.m_array && m_Variant == rhs.m_Variant && m_sCharPtr == rhs.m_sCharPtr && m_sString == rhs.m_sString && m_sStringView == rhs.m_sStringView; }

  const char* GetCharPtr() const { return m_sCharPtr.GetData(); }
  void SetCharPtr(const char* szSz) { m_sCharPtr = szSz; }

  const ezString& GetString() const { return m_sString; }
  void SetString(const ezString& sStr) { m_sString = sStr; }

  ezStringView GetStringView() const { return m_sStringView.GetView(); }
  void SetStringView(ezStringView sStrView) { m_sStringView = sStrView; }

  ezTime m_Time;
  ezEnum<ezExampleEnum> m_enumClass;
  ezBitflags<ezExampleBitflags> m_bitflagsClass;
  ezHybridArray<float, 4> m_array;
  ezVariant m_Variant;

private:
  ezString m_sCharPtr;
  ezString m_sString;
  ezString m_sStringView;
};


struct ezTestClass2Allocator : public ezRTTIAllocator
{
  virtual ezInternal::NewInstance<void> AllocateInternal(ezAllocator* pAllocator) override
  {
    ++m_iAllocs;

    return EZ_DEFAULT_NEW(ezTestClass2);
  }

  virtual void Deallocate(void* pObject, ezAllocator* pAllocator) override
  {
    ++m_iDeallocs;

    ezTestClass2* pPointer = (ezTestClass2*)pObject;
    EZ_DEFAULT_DELETE(pPointer);
  }

  static ezInt32 m_iAllocs;
  static ezInt32 m_iDeallocs;
};


class ezTestClass2b : ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestClass2b, ezReflectedClass);

public:
  ezTestClass2b() { m_sText = "Tut"; }

  const char* GetText() const { return m_sText.GetData(); }
  void SetText(const char* szSz) { m_sText = szSz; }

  ezTestStruct3 m_Struct;
  ezColor m_Color;

private:
  ezString m_sText;
};


class ezTestArrays : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestArrays, ezReflectedClass);

public:
  ezTestArrays() = default;

  bool operator==(const ezTestArrays& rhs) const
  {
    return m_Hybrid == rhs.m_Hybrid && m_Dynamic == rhs.m_Dynamic && m_Deque == rhs.m_Deque && m_HybridChar == rhs.m_HybridChar && m_CustomVariant == rhs.m_CustomVariant;
  }

  bool operator!=(const ezTestArrays& rhs) const { return !(*this == rhs); }

  ezUInt32 GetCount() const;
  double GetValue(ezUInt32 uiIndex) const;
  void SetValue(ezUInt32 uiIndex, double value);
  void Insert(ezUInt32 uiIndex, double value);
  void Remove(ezUInt32 uiIndex);

  ezUInt32 GetCountChar() const;
  const char* GetValueChar(ezUInt32 uiIndex) const;
  void SetValueChar(ezUInt32 uiIndex, const char* value);
  void InsertChar(ezUInt32 uiIndex, const char* value);
  void RemoveChar(ezUInt32 uiIndex);

  ezUInt32 GetCountDyn() const;
  const ezTestStruct3& GetValueDyn(ezUInt32 uiIndex) const;
  void SetValueDyn(ezUInt32 uiIndex, const ezTestStruct3& value);
  void InsertDyn(ezUInt32 uiIndex, const ezTestStruct3& value);
  void RemoveDyn(ezUInt32 uiIndex);

  ezUInt32 GetCountDeq() const;
  const ezTestArrays& GetValueDeq(ezUInt32 uiIndex) const;
  void SetValueDeq(ezUInt32 uiIndex, const ezTestArrays& value);
  void InsertDeq(ezUInt32 uiIndex, const ezTestArrays& value);
  void RemoveDeq(ezUInt32 uiIndex);

  ezUInt32 GetCountCustom() const;
  ezVarianceTypeAngle GetValueCustom(ezUInt32 uiIndex) const;
  void SetValueCustom(ezUInt32 uiIndex, ezVarianceTypeAngle value);
  void InsertCustom(ezUInt32 uiIndex, ezVarianceTypeAngle value);
  void RemoveCustom(ezUInt32 uiIndex);

  ezHybridArray<double, 5> m_Hybrid;
  ezHybridArray<ezString, 2> m_HybridChar;
  ezDynamicArray<ezTestStruct3> m_Dynamic;
  ezDeque<ezTestArrays> m_Deque;
  ezHybridArray<ezVarianceTypeAngle, 1> m_CustomVariant;
};


class ezTestSets : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestSets, ezReflectedClass);

public:
  ezTestSets() = default;

  bool operator==(const ezTestSets& rhs) const
  {
    return m_SetMember == rhs.m_SetMember && m_SetAccessor == rhs.m_SetAccessor && m_Deque == rhs.m_Deque && m_Array == rhs.m_Array && m_CustomVariant == rhs.m_CustomVariant;
  }

  bool operator!=(const ezTestSets& rhs) const { return !(*this == rhs); }

  const ezSet<double>& GetSet() const;
  void Insert(double value);
  void Remove(double value);

  const ezHashSet<ezInt64>& GetHashSet() const;
  void HashInsert(ezInt64 value);
  void HashRemove(ezInt64 value);

  const ezDeque<int>& GetPseudoSet() const;
  void PseudoInsert(int value);
  void PseudoRemove(int value);

  ezArrayPtr<const ezString> GetPseudoSet2() const;
  void PseudoInsert2(const ezString& value);
  void PseudoRemove2(const ezString& value);

  void PseudoInsert2b(const char* value);
  void PseudoRemove2b(const char* value);

  const ezHashSet<ezVarianceTypeAngle>& GetCustomHashSet() const;
  void CustomHashInsert(ezVarianceTypeAngle value);
  void CustomHashRemove(ezVarianceTypeAngle value);

  ezSet<ezInt8> m_SetMember;
  ezSet<double> m_SetAccessor;

  ezHashSet<ezInt32> m_HashSetMember;
  ezHashSet<ezInt64> m_HashSetAccessor;

  ezDeque<int> m_Deque;
  ezDynamicArray<ezString> m_Array;
  ezHashSet<ezVarianceTypeAngle> m_CustomVariant;
};


class ezTestMaps : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestMaps, ezReflectedClass);

public:
  ezTestMaps() = default;

  bool operator==(const ezTestMaps& rhs) const;

  const ezMap<ezString, ezInt64>& GetContainer() const;
  void Insert(const char* szKey, ezInt64 value);
  void Remove(const char* szKey);

  const ezHashTable<ezString, ezString>& GetContainer2() const;
  void Insert2(const char* szKey, const ezString& value);
  void Remove2(const char* szKey);

  const ezRangeView<const char*, ezUInt32> GetKeys3() const;
  void Insert3(const char* szKey, const ezVariant& value);
  void Remove3(const char* szKey);
  bool GetValue3(const char* szKey, ezVariant& out_value) const;

  ezMap<ezString, int> m_MapMember;
  ezMap<ezString, ezInt64> m_MapAccessor;

  ezHashTable<ezString, double> m_HashTableMember;
  ezHashTable<ezString, ezString> m_HashTableAccessor;

  ezMap<ezString, ezVarianceTypeAngle> m_CustomVariant;

  struct Tuple
  {
    ezString m_Key;
    ezVariant m_Value;
  };
  ezHybridArray<Tuple, 2> m_Accessor3;
};

class ezTestPtr : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestPtr, ezReflectedClass);

public:
  ezTestPtr()
  {
    m_pArrays = nullptr;
    m_pArraysDirect = nullptr;
  }

  ~ezTestPtr()
  {
    EZ_DEFAULT_DELETE(m_pArrays);
    EZ_DEFAULT_DELETE(m_pArraysDirect);
    for (auto ptr : m_ArrayPtr)
    {
      EZ_DEFAULT_DELETE(ptr);
    }
    m_ArrayPtr.Clear();
    for (auto ptr : m_SetPtr)
    {
      EZ_DEFAULT_DELETE(ptr);
    }
    m_SetPtr.Clear();
  }

  bool operator==(const ezTestPtr& rhs) const
  {
    if (m_sString != rhs.m_sString || (m_pArrays != rhs.m_pArrays && *m_pArrays != *rhs.m_pArrays))
      return false;

    if (m_ArrayPtr.GetCount() != rhs.m_ArrayPtr.GetCount())
      return false;

    for (ezUInt32 i = 0; i < m_ArrayPtr.GetCount(); i++)
    {
      if (!(*m_ArrayPtr[i] == *rhs.m_ArrayPtr[i]))
        return false;
    }

    // only works for the test data if the test.
    if (m_SetPtr.IsEmpty() && rhs.m_SetPtr.IsEmpty())
      return true;

    if (m_SetPtr.GetCount() != 1 || rhs.m_SetPtr.GetCount() != 1)
      return true;

    return *m_SetPtr.GetIterator().Key() == *rhs.m_SetPtr.GetIterator().Key();
  }

  void SetString(const char* szValue) { m_sString = szValue; }
  const char* GetString() const { return m_sString; }

  void SetArrays(ezTestArrays* pValue) { m_pArrays = pValue; }
  ezTestArrays* GetArrays() const { return m_pArrays; }


  ezString m_sString;
  ezTestArrays* m_pArrays;
  ezTestArrays* m_pArraysDirect;
  ezDeque<ezTestArrays*> m_ArrayPtr;
  ezSet<ezTestSets*> m_SetPtr;
};


struct ezTestEnumStruct
{
  EZ_ALLOW_PRIVATE_PROPERTIES(ezTestEnumStruct);

public:
  ezTestEnumStruct()
  {
    m_enum = ezExampleEnum::Value1;
    m_enumClass = ezExampleEnum::Value1;
    m_Enum2 = ezExampleEnum::Value1;
    m_EnumClass2 = ezExampleEnum::Value1;
  }

  bool operator==(const ezTestEnumStruct& rhs) const { return m_Enum2 == rhs.m_Enum2 && m_enum == rhs.m_enum && m_enumClass == rhs.m_enumClass && m_EnumClass2 == rhs.m_EnumClass2; }

  ezExampleEnum::Enum m_enum;
  ezEnum<ezExampleEnum> m_enumClass;

  void SetEnum(ezExampleEnum::Enum e) { m_Enum2 = e; }
  ezExampleEnum::Enum GetEnum() const { return m_Enum2; }
  void SetEnumClass(ezEnum<ezExampleEnum> e) { m_EnumClass2 = e; }
  ezEnum<ezExampleEnum> GetEnumClass() const { return m_EnumClass2; }

private:
  ezExampleEnum::Enum m_Enum2;
  ezEnum<ezExampleEnum> m_EnumClass2;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTestEnumStruct);


struct ezTestBitflagsStruct
{
  EZ_ALLOW_PRIVATE_PROPERTIES(ezTestBitflagsStruct);

public:
  ezTestBitflagsStruct()
  {
    m_bitflagsClass = ezExampleBitflags::Value1;
    m_BitflagsClass2 = ezExampleBitflags::Value1;
  }

  bool operator==(const ezTestBitflagsStruct& rhs) const { return m_bitflagsClass == rhs.m_bitflagsClass && m_BitflagsClass2 == rhs.m_BitflagsClass2; }

  ezBitflags<ezExampleBitflags> m_bitflagsClass;

  void SetBitflagsClass(ezBitflags<ezExampleBitflags> e) { m_BitflagsClass2 = e; }
  ezBitflags<ezExampleBitflags> GetBitflagsClass() const { return m_BitflagsClass2; }

private:
  ezBitflags<ezExampleBitflags> m_BitflagsClass2;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTestBitflagsStruct);
