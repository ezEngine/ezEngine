#pragma once

/// \file

/// \brief A custom enum implementation that allows to define the underlying storage type to control its memory footprint.
///
/// Advantages over a simple C++ enum:
/// 1) Storage type can be defined
/// 2) Enum is default initialized automatically
/// 3) Definition of the enum itself, the storage type and the default init value is in one place
/// 4) It makes function definitions shorter, instead of:
///      void function(ezExampleEnumBase::Enum value)
///    you can write:
///      void function(ezExampleEnum value)
/// 5) In all other ways it works exactly like a C++ enum
///
/// Example:
///
/// struct ezExampleEnumBase
/// {
///   typedef ezUInt8 StorageType;
///
///   enum Enum
///   {
///     Value1 = 1,          // normal value
///     Value2 = 2,          // normal value
///     Value3 = 3,          // normal value
///     Default = Value1 // Default initialization value (required)
///   };
/// };
/// typedef ezEnum<ezExampleEnumBase> ezExampleEnum;
///
/// This defines an "ezExampleEnum" which is stored in an ezUInt8 and is default initialized with Value1
/// For more examples see the enum test.
template <typename Derived> 
struct ezEnum : public Derived
{
public:
  typedef ezEnum<Derived> SelfType;
  typedef typename Derived::StorageType StorageType;

  /// \brief Default constructor
  EZ_FORCE_INLINE ezEnum() : m_value(Derived::Default) {} // [tested]

  /// \brief Construct from a C++ enum, and implicit conversion from enum type
  EZ_FORCE_INLINE ezEnum(typename Derived::Enum init) : m_value(init) {} // [tested]

  /// \brief Assignment operator
  EZ_FORCE_INLINE void operator= (const SelfType& rh) // [tested]
  {
    m_value = rh.m_value;
  }

  /// \brief Assignment operator.
  EZ_FORCE_INLINE void operator= (const typename Derived::Enum value) // [tested]
  {
    m_value = value;
  }

  /// \brief Implicit conversion to enum type.
  EZ_FORCE_INLINE operator typename Derived::Enum() const // [tested]
  {
    return static_cast<typename Derived::Enum>(m_value);
  }

private:
  StorageType m_value;
};


#define EZ_ENUM_VALUE_TO_STRING(name) \
  case name: return EZ_STRINGIZE(name);

/// \brief Helper macro to generate a 'ToString' function for enum values.
///
/// Usage: EZ_ENUM_TO_STRING(Value1, Value2, Value3, Value4)
/// Embed it into a struct (which defines the enums).
/// Example:
/// struct ezExampleEnum
/// {
///   enum Enum
///   {
///     A,
///     B,
///     C,
///   };
///
///   EZ_ENUM_TO_STRING(A, B, C);
/// };
#define EZ_ENUM_TO_STRING(...) \
  const char* ToString(ezUInt32 value) \
  { \
    switch (value) \
    { \
      EZ_EXPAND_ARGS(EZ_ENUM_VALUE_TO_STRING, ##__VA_ARGS__) \
      default: return nullptr; \
    } \
  }

