#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Utilities/ConversionUtils.h>

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
      //ObjectPointer,
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

  ezVariant(); // [tested]
  ezVariant(const ezVariant& other);

  template <typename T>
  ezVariant(const T& value); // [tested]

  ~ezVariant();

  void operator=(const ezVariant& other); // [tested]

  template <typename T>
  void operator=(const T& value); // [tested]

  bool operator==(const ezVariant& other) const; // [tested]
  bool operator!=(const ezVariant& other) const; // [tested]
  
  template <typename T>
  bool operator==(const T& other) const; // [tested]

  template <typename T>
  bool operator!=(const T& other) const; // [tested]

  bool IsValid() const; // [tested]

  template <typename T>
  bool IsA() const; // [tested]

  Type::Enum GetType() const; // [tested]

  template <typename T>
  const T& Get() const; // [tested]

  template <typename T>
  bool CanConvertTo() const; // [tested]

  bool CanConvertTo(Type::Enum type) const; // [tested]

  template <typename T>
  T ConvertTo(bool* out_pSuccessful = NULL) const; // [tested]

  ezVariant ConvertTo(Type::Enum type, bool* out_pSuccessful = NULL) const; // [tested]

  template <typename Functor>
  static void DispatchTo(Functor& functor, Type::Enum type); // [tested]

private:

  friend class ezVariantHelper;
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

  static bool IsNumber(ezUInt32 type);
  static bool IsFloatingPoint(ezUInt32 type);

  template <typename T>
  T ConvertNumber() const;
};

typedef ezDynamicArray<ezVariant> ezVariantArray;
typedef ezHashTable<ezString, ezVariant> ezVariantDictionary;

#include <Foundation/Basics/Types/Implementation/VariantTypeDeduction_inl.h>
#include <Foundation/Basics/Types/Implementation/VariantHelper_inl.h>
#include <Foundation/Basics/Types/Implementation/Variant_inl.h>
