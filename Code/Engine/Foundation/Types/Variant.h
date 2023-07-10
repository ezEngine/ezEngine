#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Types/TypedPointer.h>
#include <Foundation/Types/Types.h>
#include <Foundation/Types/VariantType.h>

#include <Foundation/Reflection/Implementation/DynamicRTTI.h>
#include <Foundation/Utilities/ConversionUtils.h>

class ezRTTI;

/// \brief Defines a reference to an immutable object owned by an ezVariant.
///
/// Used to store custom types inside an ezVariant. As lifetime is governed by the ezVariant, it is generally not safe to store an ezTypedObject.
/// This class is needed to be able to differentiate between ezVariantType::TypedPointer and ezVariantType::TypedObject e.g. in ezVariant::DispatchTo.
/// \sa ezVariant, EZ_DECLARE_CUSTOM_VARIANT_TYPE
struct ezTypedObject
{
  EZ_DECLARE_POD_TYPE();
  const void* m_pObject = nullptr;
  const ezRTTI* m_pType = nullptr;

  bool operator==(const ezTypedObject& rhs) const
  {
    return m_pObject == rhs.m_pObject;
  }
  bool operator!=(const ezTypedObject& rhs) const
  {
    return m_pObject != rhs.m_pObject;
  }
};

/// \brief ezVariant is a class that can store different types of variables, which is useful in situations where it is not clear up front,
/// which type of data will be passed around.
///
/// The variant supports a fixed list of types that it can store (\see ezVariant::Type). All types of 16 bytes or less in size can be stored
/// without requiring a heap allocation. For larger types memory is allocated on the heap. In general variants should be used for code that
/// needs to be flexible. Although ezVariant is implemented very efficiently, it should be avoided to use ezVariant in code that needs to be
/// fast.
class EZ_FOUNDATION_DLL ezVariant
{
public:
  using Type = ezVariantType;
  template <typename T>
  using TypeDeduction = ezVariantTypeDeduction<T>;

  /// \brief helper struct to wrap a string pointer
  struct StringWrapper
  {
    EZ_ALWAYS_INLINE StringWrapper(const char* szStr)
      : m_str(szStr)
    {
    }
    const char* m_str;
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
  ezVariant(ezVariant&& other) noexcept; // [tested]

  ezVariant(const bool& value);
  ezVariant(const ezInt8& value);
  ezVariant(const ezUInt8& value);
  ezVariant(const ezInt16& value);
  ezVariant(const ezUInt16& value);
  ezVariant(const ezInt32& value);
  ezVariant(const ezUInt32& value);
  ezVariant(const ezInt64& value);
  ezVariant(const ezUInt64& value);
  ezVariant(const float& value);
  ezVariant(const double& value);
  ezVariant(const ezColor& value);
  ezVariant(const ezVec2& value);
  ezVariant(const ezVec3& value);
  ezVariant(const ezVec4& value);
  ezVariant(const ezVec2I32& value);
  ezVariant(const ezVec3I32& value);
  ezVariant(const ezVec4I32& value);
  ezVariant(const ezVec2U32& value);
  ezVariant(const ezVec3U32& value);
  ezVariant(const ezVec4U32& value);
  ezVariant(const ezQuat& value);
  ezVariant(const ezMat3& value);
  ezVariant(const ezMat4& value);
  ezVariant(const ezTransform& value);
  ezVariant(const char* value);
  ezVariant(const ezString& value);
  ezVariant(const ezUntrackedString& value);
  ezVariant(const ezStringView& value, bool bCopyString = true);
  ezVariant(const ezDataBuffer& value);
  ezVariant(const ezTime& value);
  ezVariant(const ezUuid& value);
  ezVariant(const ezAngle& value);
  ezVariant(const ezColorGammaUB& value);
  ezVariant(const ezHashedString& value);
  ezVariant(const ezTempHashedString& value);

  ezVariant(const ezVariantArray& value);
  ezVariant(const ezVariantDictionary& value);

  ezVariant(const ezTypedPointer& value);
  ezVariant(const ezTypedObject& value);

  template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::CustomTypeCast, int> = 0>
  ezVariant(const T& value);

  template <typename T>
  ezVariant(const T* value);

  /// \brief Initializes to a TypedPointer of the given object and type.
  ezVariant(void* value, const ezRTTI* pType);

  /// \brief Initializes to a TypedObject by cloning the given object and type.
  void CopyTypedObject(const void* value, const ezRTTI* pType); // [tested]

  /// \brief Initializes to a TypedObject by taking ownership of the given object and type.
  void MoveTypedObject(void* value, const ezRTTI* pType); // [tested]

  /// \brief If necessary, this will deallocate any heap memory that is not in use any more.
  ~ezVariant();

  /// \brief Copies the data from the \a other variant into this one.
  void operator=(const ezVariant& other); // [tested]

  /// \brief Moves the data from the \a other variant into this one.
  void operator=(ezVariant&& other) noexcept; // [tested]

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

  /// \brief Returns whether the stored type is numerical type either integer or floating point.
  ///
  /// Bool counts as number.
  bool IsNumber() const; // [tested]

  /// \brief Returns whether the stored type is floating point (float or double).
  bool IsFloatingPoint() const; // [tested]

  /// \brief Returns whether the stored type is a string (ezString or ezStringView).
  bool IsString() const; // [tested]

  /// \brief Returns whether the stored type is a hashed string (ezHashedString or ezTempHashedString).
  bool IsHashedString() const;

  /// \brief Returns whether the stored type is exactly the given type.
  ///
  /// \note This explicitly also differentiates between the different integer types.
  /// So when the variant stores an Int32, IsA<Int64>() will return false, even though the types could be converted.
  template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::DirectCast, int> = 0>
  bool IsA() const; // [tested]

  template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::PointerCast, int> = 0>
  bool IsA() const; // [tested]

  template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::TypedObject, int> = 0>
  bool IsA() const; // [tested]

  template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::CustomTypeCast, int> = 0>
  bool IsA() const; // [tested]

  /// \brief Returns the exact ezVariant::Type value.
  Type::Enum GetType() const; // [tested]

  /// \brief Returns the variants value as the provided type.
  ///
  /// \note This function does not do ANY type of conversion from the stored type to the given type. Not even integer conversions!
  /// If the types don't match, this function will assert!
  /// So be careful to use this function only when you know exactly that the stored type matches the expected type.
  ///
  /// Prefer to use ConvertTo() when you can instead.
  template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::DirectCast, int> = 0>
  const T& Get() const; // [tested]

  template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::PointerCast, int> = 0>
  T Get() const; // [tested]

  template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::TypedObject, int> = 0>
  const T Get() const; // [tested]

  template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::CustomTypeCast, int> = 0>
  const T& Get() const; // [tested]

  /// \brief Returns an writable ezTypedPointer to the internal data.
  /// If the data is currently shared a clone will be made to ensure we hold the only reference.
  ezTypedPointer GetWriteAccess(); // [tested]

  template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::DirectCast, int> = 0>
  T& GetWritable(); // [tested]

  template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::PointerCast, int> = 0>
  T GetWritable(); // [tested]

  template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::CustomTypeCast, int> = 0>
  T& GetWritable(); // [tested]


  /// \brief Returns a const void* to the internal data.
  /// For TypedPointer and TypedObject this will return a pointer to the target object.
  const void* GetData() const; // [tested]

  /// \brief Returns the ezRTTI type of the held value.
  /// For TypedPointer and TypedObject this will return the type of the target object.
  const ezRTTI* GetReflectedType() const; // [tested]

  /// \brief Returns the sub value at iIndex. This could be an element in an array or a member property inside a reflected type.
  ///
  /// Out of bounds access is handled gracefully and will return an invalid variant.
  const ezVariant operator[](ezUInt32 uiIndex) const; // [tested]

  /// \brief Returns the sub value with szKey. This could be a value in a dictionary or a member property inside a reflected type.
  ///
  /// This function will return an invalid variant if no corresponding sub value is found.
  const ezVariant operator[](StringWrapper key) const; // [tested]

  /// \brief Returns whether the stored type can generally be converted to the desired type.
  ///
  /// This function will return true for all number conversions, as float / double / int / etc. can generally be converted into each
  /// other. It will also return true for all conversion from string to number types, and from all 'simple' types (not array or dictionary)
  /// to string.
  ///
  /// \note This function only returns whether a conversion between the stored TYPE and the desired TYPE is generally possible. It does NOT
  /// return whether the stored VALUE is indeed convertible to the desired type. For example, a string is generally convertible to float, if
  /// it stores a string representation of a float value. If, however, it stores anything else, the conversion can still fail.
  ///
  /// The only way to figure out whether the stored data can be converted to some type, is to actually convert it, using ConvertTo(), and
  /// then to check the conversion status.
  template <typename T>
  bool CanConvertTo() const; // [tested]

  /// \brief Same as the templated CanConvertTo function.
  bool CanConvertTo(Type::Enum type) const; // [tested]

  /// \brief Tries to convert the stored value to the given type. The optional status parameter can be used to check whether the conversion
  /// succeeded.
  ///
  /// When CanConvertTo() returns false, ConvertTo() will also always fail. However, when CanConvertTo() returns true, this is no guarantee
  /// that ConvertTo() will succeed. Conversion between numbers and to strings will generally succeed. However, converting from a string to
  /// another type can fail or succeed, depending on the exact string value.
  template <typename T>
  T ConvertTo(ezResult* out_pConversionStatus = nullptr) const; // [tested]

  /// \brief Same as the templated function.
  ezVariant ConvertTo(Type::Enum type, ezResult* out_pConversionStatus = nullptr) const; // [tested]

  /// \brief This will call the overloaded operator() (function call operator) of the provided functor.
  ///
  /// This allows to implement a functor that overloads operator() for different types and then call the proper version of that operator,
  /// depending on the provided runtime type. Note that the proper overload of operator() is selected by providing a dummy type, but it will
  /// contain no useful value. Instead, store the other necessary data inside the functor object, before calling this function. For example,
  /// store a pointer to a variant inside the functor object and then call DispatchTo to execute the function that will handle the given
  /// type of the variant.
  template <typename Functor, class... Args>
  static auto DispatchTo(Functor& ref_functor, Type::Enum type, Args&&... args); // [tested]

  /// \brief Computes the hash value of the stored data. Returns uiSeed (unchanged) for an invalid Variant.
  ezUInt64 ComputeHash(ezUInt64 uiSeed = 0) const;

private:
  friend class ezVariantHelper;
  friend struct CompareFunc;
  friend struct GetTypeFromVariantFunc;

  struct SharedData
  {
    void* m_Ptr;
    const ezRTTI* m_pType;
    ezAtomicInteger32 m_uiRef = 1;
    EZ_ALWAYS_INLINE SharedData(void* pPtr, const ezRTTI* pType)
      : m_Ptr(pPtr)
      , m_pType(pType)
    {
    }
    virtual ~SharedData() = default;
    virtual SharedData* Clone() const = 0;
  };

  template <typename T>
  class TypedSharedData : public SharedData
  {
  private:
    T m_t;

  public:
    EZ_ALWAYS_INLINE TypedSharedData(const T& value, const ezRTTI* pType = nullptr)
      : SharedData(&m_t, pType)
      , m_t(value)
    {
    }

    virtual SharedData* Clone() const override
    {
      return EZ_DEFAULT_NEW(TypedSharedData<T>, m_t, m_pType);
    }
  };

  class RTTISharedData : public SharedData
  {
  public:
    RTTISharedData(void* pData, const ezRTTI* pType);

    ~RTTISharedData();

    virtual SharedData* Clone() const override;
  };

  struct InlinedStruct
  {
    constexpr static int DataSize = 4 * sizeof(float) - sizeof(void*);
    ezUInt8 m_Data[DataSize];
    const ezRTTI* m_pType;
  };

  union Data
  {
    float f[4];
    SharedData* shared;
    InlinedStruct inlined;
  } m_Data;

  ezUInt32 m_uiType : 31;
  ezUInt32 m_bIsShared : 1; // NOLINT(ez*)

  template <typename T>
  void InitInplace(const T& value);

  template <typename T>
  void InitShared(const T& value);

  template <typename T>
  void InitTypedObject(const T& value, ezTraitInt<0>);
  template <typename T>
  void InitTypedObject(const T& value, ezTraitInt<1>);

  void InitTypedPointer(void* value, const ezRTTI* pType);

  void Release();
  void CopyFrom(const ezVariant& other);
  void MoveFrom(ezVariant&& other);

  template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::DirectCast, int> = 0>
  const T& Cast() const;
  template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::PointerCast, int> = 0>
  T Cast() const;
  template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::TypedObject, int> = 0>
  const T Cast() const;
  template <typename T, typename std::enable_if_t<ezVariantTypeDeduction<T>::classification == ezVariantClass::CustomTypeCast, int> = 0>
  const T& Cast() const;

  static bool IsNumberStatic(ezUInt32 type);
  static bool IsFloatingPointStatic(ezUInt32 type);
  static bool IsStringStatic(ezUInt32 type);
  static bool IsHashedStringStatic(ezUInt32 type);
  static bool IsVector2Static(ezUInt32 type);
  static bool IsVector3Static(ezUInt32 type);
  static bool IsVector4Static(ezUInt32 type);

  // Needed to prevent including ezRTTI in ezVariant.h
  static bool IsDerivedFrom(const ezRTTI* pType1, const ezRTTI* pType2);
  static ezStringView GetTypeName(const ezRTTI* pType);

  template <typename T>
  T ConvertNumber() const;
};

/// \brief An overload of ezDynamicCast for dynamic casting a variant to a pointer type.
///
/// If the ezVariant stores an ezTypedPointer pointer, this pointer will be dynamically cast to T*.
/// If the ezVariant stores any other type (or nothing), nullptr is returned.
template <typename T>
EZ_ALWAYS_INLINE T ezDynamicCast(const ezVariant& variant)
{
  if (variant.IsA<T>())
  {
    return variant.Get<T>();
  }

  return nullptr;
}

#include <Foundation/Types/Implementation/VariantHelper_inl.h>

#include <Foundation/Types/Implementation/Variant_inl.h>
