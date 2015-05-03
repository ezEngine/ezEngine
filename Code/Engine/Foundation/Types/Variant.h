#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Utilities/ConversionUtils.h>

class ezReflectedClass;

/// \brief ezVariant is a class that can store different types of variables, which is useful in situations where it is not clear up front, which type of data will be passed around.
///
/// The variant supports a fixed list of types that it can store (\see ezVariant::Type). All types of 16 bytes or less in size can be stored without
/// requiring a heap allocation. For larger types memory is allocated on the heap.
/// In general variants should be used for code that needs to be flexible. Although ezVariant is implemented very efficiently, it should be avoided
/// to use ezVariant in code that needs to be fast.
class EZ_FOUNDATION_DLL ezVariant
{
public:
  /// \brief This enum describes the type of data that is currently stored inside the variant.
  struct Type
  {
    typedef ezUInt8 StorageType;
    /// \brief This enum describes the type of data that is currently stored inside the variant.
    enum Enum
    {
      Invalid,            ///< The variant stores no (valid) data at the moment.
      Bool,               ///< The variant stores a bool.
      Int8,               ///< The variant stores an ezInt8.
      UInt8,              ///< The variant stores an ezUInt8.
      Int16,              ///< The variant stores an ezInt16.
      UInt16,             ///< The variant stores an ezUInt16.
      Int32,              ///< The variant stores an ezInt32.
      UInt32,             ///< The variant stores an ezUInt32.
      Int64,              ///< The variant stores an ezInt64.
      UInt64,             ///< The variant stores an ezUInt64.
      Float,              ///< The variant stores a float.
      Double,             ///< The variant stores a double.
      Color,              ///< The variant stores an ezColor.
      Vector2,            ///< The variant stores an ezVec2.
      Vector3,            ///< The variant stores an ezVec3.
      Vector4,            ///< The variant stores an ezVec4.
      Quaternion,         ///< The variant stores an ezQuat.
      Matrix3,            ///< The variant stores an ezMat3. A heap allocation is required to store this data type.
      Matrix4,            ///< The variant stores an ezMat4. A heap allocation is required to store this data type.
      String,             ///< The variant stores a string. A heap allocation is required to store this data type.
      Time,               ///< The variant stores an ezTime value.
      Uuid,               ///< The variant stores an ezUuid value.
      VariantArray,       ///< The variant stores an array of ezVariant's. A heap allocation is required to store this data type.
      VariantDictionary,  ///< The variant stores a dictionary (hashmap) of ezVariant's. A heap allocation is required to store this data type.
      ReflectedPointer,   ///< The variant stores a pointer to a dynamically reflected object.
      VoidPointer,        ///< The variant stores a void pointer.
      ENUM_COUNT,         ///< Number of values for ezVariant::Type.
      Default = Invalid   ///< Default value used by ezEnum.
    };
  };

  /// \brief A helper struct to convert the C++ type, which is passed as the template argument, into one of the ezVariant::Type enum values.
  template <typename T>
  struct TypeDeduction
  {
    enum
    {
      value = Type::Invalid,
      forceSharing = false,
      hasReflectedMembers = false
    };

    typedef T StorageType;
  };
  
  /// \brief Initializes the variant to be 'Invalid'
  ezVariant(); // [tested]

  /// \brief Copies the data from the other variant.
  ///
  /// \note If the data of the variant needed to be allocated on the heap, it will be shared among variants.
  /// Thus, once you have stored such a type inside a variant, you can copy it to other variants, without introducing
  /// additional memory allocations.
  ezVariant(const ezVariant& other); // [tested]

  /// \brief Moves the data from the other variant.
  ezVariant(ezVariant&& other); // [tested]

  /// \brief Deduces the type of the passed argument and stores that type in the variant.
  ///
  /// If the type to be stored in the variant is not supported, a compile time error will occur.
  template <typename T>
  ezVariant(const T& value); // [tested]

  /// \brief If necessary, this will deallocate any heap memory that is not in use any more.
  ~ezVariant();

  /// \brief Copies the data from the \a other variant into this one.
  void operator=(const ezVariant& other); // [tested]

  /// \brief Moves the data from the \a other variant into this one.
  void operator=(ezVariant&& other); // [tested]

  /// \brief Deduces the type of \a T and stores \a value.
  ///
  /// If the type to be stored in the variant is not supported, a compile time error will occur.
  template <typename T>
  void operator=(const T& value); // [tested]

  /// \brief Will compare the value of this variant to that of \a other.
  ///
  /// If both variants store 'numbers' (float, double, int types) the comparison will work, even if the types are not identical.
  ///
  /// \note If the two types are not numbers and not equal, an assert will occur. So be careful to only compare variants
  /// that can either both be converted to double (\see CanConvertTo()) or whose types are equal.
  bool operator==(const ezVariant& other) const; // [tested]

  /// \brief Same as operator== (with a twist!)
  bool operator!=(const ezVariant& other) const; // [tested]
  
  /// \brief See non-templated operator==
  template <typename T>
  bool operator==(const T& other) const; // [tested]

  /// \brief See non-templated operator!=
  template <typename T>
  bool operator!=(const T& other) const; // [tested]

  /// \brief Returns whether this variant stores any other type than 'Invalid'.
  bool IsValid() const; // [tested]

  /// \brief Returns whether the stored type is exactly the given type.
  ///
  /// \note This explicitly also differentiates between the different integer types.
  /// So when the variant stores an Int32, IsA<Int64>() will return false, even though the types could be converted.
  template <typename T>
  bool IsA() const; // [tested]

  /// \brief Returns the exact ezVariant::Type value.
  Type::Enum GetType() const; // [tested]

  /// \brief Returns the variants value as the provided type.
  ///
  /// \note This function does not do ANY type of conversion from the stored type to the given type. Not even integer conversions!
  /// If the types don't match, this function will assert!
  /// So be careful to use this function only when you know exactly that the stored type matches the expected type.
  ///
  /// Prefer to use ConvertTo() when you instead.
  template <typename T>
  const T& Get() const; // [tested]

  /// \brief Returns a void* to the internal data.
  void* GetData(); // [tested]

  /// \brief Returns a void* to the internal data.
  const void* GetData() const; // [tested]

  /// \brief Returns the sub value at iIndex. This could be an element in an array or a member property inside a reflected type.
  ///
  /// Out of bounds access is handled gracefully and will return an invalid variant.
  ezVariant operator[](ezUInt32 uiIndex) const; // [tested]

  /// \brief Returns the sub value with szKey. This could be a value in a dictionary or a member property inside a reflected type.
  ///
  /// This function will return an invalid variant if no corresponding sub value is found.
  ezVariant operator[](ezHashing::StringWrapper szKey) const; // [tested]

  /// \brief Returns whether the stored type can generally be converted to the desired type.
  ///
  /// This function will return true for all number conversions, as float / double / int / etc. can generally be converted into each
  /// other. It will also return true for all conversion from string to number types, and from all 'simple' types (not array or dictionary) to string.
  ///
  /// \note This function only returns whether a conversion between the stored TYPE and the desired TYPE is generally possible. It does NOT return
  /// whether the stored VALUE is indeed convertible to the desired type. For example, a string is generally convertible to float, if it stores
  /// a string representation of a float value. If, however, it stores anything else, the conversion can still fail.
  ///
  /// The only way to figure out whether the stored data can be converted to some type, is to actually convert it, using ConvertTo(), and then
  /// to check the conversion status.
  template <typename T>
  bool CanConvertTo() const; // [tested]

  /// \brief Same as the templated CanConvertTo function.
  bool CanConvertTo(Type::Enum type) const; // [tested]

  /// \brief Tries to convert the stored value to the given type. The optional status parameter can be used to check whether the conversion succeeded.
  ///
  /// When CanConvertTo() returns false, ConvertTo() will also always fail. However, when CanConvertTo() returns true, this is no guarantee that
  /// ConvertTo() will succeed. Conversion between numbers and to strings will generally succeed. However, converting from a string to another type
  /// can fail or succeed, depending on the exact string value.
  template <typename T>
  T ConvertTo(ezResult* out_pConversionStatus = nullptr) const; // [tested]

  /// \brief Same as the templated function.
  ezVariant ConvertTo(Type::Enum type, ezResult* out_pConversionStatus = nullptr) const; // [tested]

  /// \brief This will call the overloaded operator() (function call operator) of the provided functor.
  ///
  /// This allows to implement a functor that overloads operator() for different types and then call the proper version of that operator, depending on
  /// the provided runtime type. Note that the proper overload of operator() is selected by providing a dummy type, but it will contain no useful value.
  /// Instead, store the other necessary data inside the functor object, before calling this function. For example, store a pointer to a variant inside
  /// the functor object and then call DispatchTo to execute the function that will handle the given type of the variant.
  template <typename Functor>
  static void DispatchTo(Functor& functor, Type::Enum type); // [tested]

  /// \brief Computes the hash value of the stored data. Returns uiSeed (unchanged) for an invalid Variant.
  ezUInt64 ComputeHash(ezUInt64 uiSeed = 0) const;

private:

  friend class ezVariantHelper;
  friend struct CompareFunc;

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
  void Store(const T& value, ezTraitInt<0>); // in-place storage

  template <typename StorageType, typename T>
  void Store(const T& value, ezTraitInt<1>); // shared storage

  void Release();
  void CopyFrom(const ezVariant& other);
  void MoveFrom(ezVariant&& other);

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
typedef ezVariant::Type ezVariantType;

#include <Foundation/Types/Implementation/VariantTypeDeduction_inl.h>
#include <Foundation/Types/Implementation/VariantHelper_inl.h>
#include <Foundation/Types/Implementation/Variant_inl.h>

