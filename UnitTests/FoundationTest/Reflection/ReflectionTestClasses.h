#pragma once

#include <PCH.h>
#include <Foundation/Reflection/Reflection.h>

struct ezExampleEnum
{
  typedef ezInt8 StorageType;
  enum Enum
  {
    Value1 = 1,          // normal value
    Value2 = -2,         // normal value
    Value3 = 4,          // normal value
    Default = Value1     // Default initialization value (required)
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


class ezAbstractTestClass : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAbstractTestClass);

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
  ezTestStruct()
  {
    m_fFloat1 = 1.1f;
    m_iInt2 = 2;
    m_vProperty3.Set(3, 4, 5);
    m_UInt8 = 6;
    m_variant = "Test";
  }

  bool operator==(const ezTestStruct& rhs) const
  {
    return m_fFloat1 == rhs.m_fFloat1 &&
      m_UInt8 == rhs.m_UInt8 &&
      m_variant == rhs.m_variant &&
      m_iInt2 == rhs.m_iInt2 &&
      m_vProperty3 == rhs.m_vProperty3;
  }

  float m_fFloat1;
  ezUInt8 m_UInt8;
  ezVariant m_variant;

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
    m_iInt2 = 2;
    m_UInt8 = 6;
  }

  bool operator==(const ezTestStruct3& rhs) const
  {
    return m_fFloat1 == rhs.m_fFloat1 && m_iInt2 == rhs.m_iInt2 && m_UInt8 == rhs.m_UInt8;
  }

  bool operator!=(const ezTestStruct3& rhs) const
  {
    return !(*this == rhs);
  }

  double m_fFloat1;
  ezInt16 m_UInt8;

private:
  void SetInt(ezUInt32 i) { m_iInt2 = i; }
  ezUInt32 GetInt() const { return m_iInt2; }

  ezInt32 m_iInt2;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTestStruct3);


class ezTestClass1 : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestClass1);

public:
  ezTestClass1()
  {
    m_MyVector.Set(3, 4, 5);

    m_Struct.m_fFloat1 = 33.3f;

    m_Color = ezColor::CornflowerBlue; // The Original!
  }

  bool operator==(const ezTestClass1& rhs) const
  {
    return m_Struct == rhs.m_Struct &&
      m_MyVector == rhs.m_MyVector &&
      m_Color == rhs.m_Color;
  }

  ezVec3 GetVector() const { return m_MyVector; }

  ezTestStruct m_Struct;
  ezVec3 m_MyVector;
  ezColor m_Color;
};


class ezTestClass2 : public ezTestClass1
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestClass2);

public:
  ezTestClass2()
  {
    m_Text = "Legen";
  }

  bool operator==(const ezTestClass2& rhs) const
  {
    return m_Time == rhs.m_Time &&
      m_enumClass == rhs.m_enumClass &&
      m_bitflagsClass == rhs.m_bitflagsClass &&
      m_array == rhs.m_array &&
      m_Variant == rhs.m_Variant &&
      m_Text == rhs.m_Text;
  }

  const char* GetText() const { return m_Text.GetData(); }
  void SetText(const char* sz) { m_Text = sz; }

  ezTime m_Time;
  ezEnum<ezExampleEnum> m_enumClass;
  ezBitflags<ezExampleBitflags> m_bitflagsClass;
  ezHybridArray<float, 4> m_array;
  ezVariant m_Variant;

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


class ezTestArrays : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestArrays);

public:
  ezTestArrays()
  {
  }

  bool operator==(const ezTestArrays& rhs) const
  {
    return m_Hybrid == rhs.m_Hybrid && m_Dynamic == rhs.m_Dynamic && m_Deque == rhs.m_Deque;
  }

  bool operator!=(const ezTestArrays& rhs) const
  {
    return !(*this == rhs);
  }

  ezUInt32 GetCount() const;
  double GetValue(ezUInt32 uiIndex) const;
  void SetValue(ezUInt32 uiIndex, double value);
  void Insert(ezUInt32 uiIndex, double value);
  void Remove(ezUInt32 uiIndex);

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

  ezHybridArray<double, 5> m_Hybrid;
  ezDynamicArray<ezTestStruct3> m_Dynamic;
  ezDeque<ezTestArrays> m_Deque;
};


class ezTestSets : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestSets);

public:
  ezTestSets()
  {
  }

  bool operator==(const ezTestSets& rhs) const
  {
    return m_SetMember == rhs.m_SetMember && m_SetAccessor == rhs.m_SetAccessor
      && m_Deque == rhs.m_Deque && m_Array == rhs.m_Array;
  }

  bool operator!=(const ezTestSets& rhs) const
  {
    return !(*this == rhs);
  }

  const ezSet<double>& GetSet() const;
  void Insert(double value);
  void Remove(double value);

  const ezDeque<int>& GetPseudoSet() const;
  void PseudoInsert(int value);
  void PseudoRemove(int value);

  ezArrayPtr<const ezString> GetPseudoSet2() const;
  void PseudoInsert2(const ezString& value);
  void PseudoRemove2(const ezString& value);

  void PseudoInsert2b(const char* value);
  void PseudoRemove2b(const char* value);

  ezSet<ezInt8> m_SetMember;
  ezSet<double> m_SetAccessor;
  
  ezDeque<int> m_Deque;
  ezDynamicArray<ezString> m_Array;
};


class ezTestPtr : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestPtr);

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
    return m_sString == rhs.m_sString &&
      (m_pArrays == rhs.m_pArrays ||
      *m_pArrays == *rhs.m_pArrays);
  }

  void SetString(const char* pzValue) { m_sString = pzValue; }
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
    m_enum2 = ezExampleEnum::Value1;
    m_enumClass2 = ezExampleEnum::Value1;
  }

  bool operator==(const ezTestEnumStruct& rhs) const
  {
    return m_enum2 == rhs.m_enum2 &&
      m_enum == rhs.m_enum &&
      m_enumClass == rhs.m_enumClass &&
      m_enumClass2 == rhs.m_enumClass2;
  }

  ezExampleEnum::Enum m_enum;
  ezEnum<ezExampleEnum> m_enumClass;

  void SetEnum(ezExampleEnum::Enum e) { m_enum2 = e; }
  ezExampleEnum::Enum GetEnum() const { return m_enum2; }
  void SetEnumClass(ezEnum<ezExampleEnum> e) { m_enumClass2 = e; }
  ezEnum<ezExampleEnum> GetEnumClass() const { return m_enumClass2; }

private:
  ezExampleEnum::Enum m_enum2;
  ezEnum<ezExampleEnum> m_enumClass2;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTestEnumStruct);


struct ezTestBitflagsStruct
{
  EZ_ALLOW_PRIVATE_PROPERTIES(ezTestBitflagsStruct);

public:
  ezTestBitflagsStruct()
  {
    m_bitflagsClass = ezExampleBitflags::Value1;
    m_bitflagsClass2 = ezExampleBitflags::Value1;
  }

  bool operator==(const ezTestBitflagsStruct& rhs) const
  {
    return m_bitflagsClass == rhs.m_bitflagsClass &&
      m_bitflagsClass2 == rhs.m_bitflagsClass2;
  }

  ezBitflags<ezExampleBitflags> m_bitflagsClass;

  void SetBitflagsClass(ezBitflags<ezExampleBitflags> e) { m_bitflagsClass2 = e; }
  ezBitflags<ezExampleBitflags> GetBitflagsClass() const { return m_bitflagsClass2; }

private:
  ezBitflags<ezExampleBitflags> m_bitflagsClass2;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTestBitflagsStruct);

