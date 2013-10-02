#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Time/Time.h>

/// \todo document and test
class EZ_FOUNDATION_DLL ezVariant
{
public:

  struct Type
  {
    enum Enum
    {
      Invalid,
      Bool,
      Int32,
      UInt32,
      Int64,
      UInt64,
      Float,
      Double,
      Color,
      Vector2,
      Vector3,
      Vector4,
      Quaternion,
      Matrix3,
      Matrix4,
      String,
      Time,
      VariantArray,
      VariantDictionary,
      ObjectPointer,
      VoidPointer
    };
  };

  template <typename T>
  struct TypeDeduction
  {
    enum
    {
      value = Type::Invalid,
      forceSharing = false
    };

    typedef T StorageType;
  };
  
  static const ezVariant Invalid;

  ezVariant();
  ezVariant(const ezVariant& other);

  template <typename T>
  ezVariant(const T& value);

  ~ezVariant();

  void operator=(const ezVariant& other);

  template <typename T>
  void operator=(const T& value);

  bool operator==(const ezVariant& other) const;
  bool operator!=(const ezVariant& other) const;
  
  template <typename T>
  bool operator==(const T& other) const;

  template <typename T>
  bool operator!=(const T& other) const;

  bool IsValid() const;

  template <typename T>
  bool IsA() const;

  Type::Enum GetType() const;

  template <typename T>
  const T& Get() const;

  template <typename T>
  bool CanConvertTo() const;

  bool CanConvertTo(Type::Enum type) const;

  template <typename T>
  T ConvertTo() const;

  ezVariant ConvertTo(Type::Enum type) const;

  template <typename Functor>
  static void DispatchTo(Functor& functor, Type::Enum type);

private:

  friend struct ezVariantConversion;
  friend struct CompareFunc;
  friend struct DestructFunc;
  friend struct CopyFunc;

  struct SharedData
  {
    void* m_Ptr;
    ezAtomicInteger32 m_uiRef;
    EZ_FORCE_INLINE SharedData(void* ptr) : m_Ptr(ptr), m_uiRef(1) { }
    virtual ~SharedData() { }
  };

  template <typename T>
  class TypedSharedData : public SharedData
  {
  private:
    T m_t;
  public:
    EZ_FORCE_INLINE TypedSharedData(const T& value) : SharedData(&m_t), m_t(value) { }
  };

  union Data
  {
    float f[4];
    SharedData* shared;
  } m_Data;

  ezUInt32 m_Type : 31;
  ezUInt32 m_bIsShared : 1;

  template <typename T>
  void Init(const T& value);

  template <typename StorageType, typename T>
  void Store(const T& value, ezTraitInt<0>);

  template <typename StorageType, typename T>
  void Store(const T& value, ezTraitInt<1>);

  void Release();
  void CopyFrom(const ezVariant& other);

  template <typename T>
  T& Cast();

  template <typename T>
  const T& Cast() const;

  template <typename T>
  T ConvertNumber() const;
};

typedef ezDynamicArray<ezVariant> ezVariantArray;
typedef ezHashTable<ezString, ezVariant> ezVariantDictionary;

#include <Foundation/Basics/Types/Implementation/VariantTypeDeduction_inl.h>
#include <Foundation/Basics/Types/Implementation/VariantConversion_inl.h>
#include <Foundation/Basics/Types/Implementation/Variant_inl.h>
