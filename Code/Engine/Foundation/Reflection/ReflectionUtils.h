#pragma once

#include <Foundation/Containers/Set.h>
#include <Foundation/Reflection/Reflection.h>

class ezVariant;
class ezAbstractProperty;

/// \brief Helper functions for handling reflection related operations.
///
/// This utility class provides high-level functions for working with ezEngine's reflection system.
/// It offers functionality for type introspection, property manipulation, enum/bitflag conversion,
/// object comparison, and dependency analysis.
///
/// Key functionality:
/// - Property value access and modification through ezVariant
/// - Enum and bitflag string conversion
/// - Type dependency analysis and sorting
/// - Object comparison and default value handling
/// - Safe property access with automatic type checking
class EZ_FOUNDATION_DLL ezReflectionUtils
{
public:
  static const ezRTTI* GetCommonBaseType(const ezRTTI* pRtti1, const ezRTTI* pRtti2);

  /// \brief Returns whether a type can be stored directly inside a ezVariant.
  static bool IsBasicType(const ezRTTI* pRtti);

  /// \brief Returns whether the property is a non-ptr basic type or custom type.
  static bool IsValueType(const ezAbstractProperty* pProp);

  /// \brief Returns the RTTI type matching the variant's type.
  static const ezRTTI* GetTypeFromVariant(const ezVariant& value);
  static const ezRTTI* GetTypeFromVariant(ezVariantType::Enum type);

  /// \brief Sets the Nth component of the vector to the given value.
  ///
  /// vector's type needs to be in between ezVariant::Type::Vector2 and ezVariant::Type::Vector4U.
  static ezUInt32 GetComponentCount(ezVariantType::Enum type);
  static void SetComponent(ezVariant& ref_vector, ezUInt32 uiComponent, double fValue);                             // [tested]
  static double GetComponent(const ezVariant& vector, ezUInt32 uiComponent);

  static ezVariant GetMemberPropertyValue(const ezAbstractMemberProperty* pProp, const void* pObject);              // [tested] via ToolsFoundation
  static void SetMemberPropertyValue(const ezAbstractMemberProperty* pProp, void* pObject, const ezVariant& value); // [tested] via ToolsFoundation

  static ezVariant GetArrayPropertyValue(const ezAbstractArrayProperty* pProp, const void* pObject, ezUInt32 uiIndex);
  static void SetArrayPropertyValue(const ezAbstractArrayProperty* pProp, void* pObject, ezUInt32 uiIndex, const ezVariant& value);

  static void InsertSetPropertyValue(const ezAbstractSetProperty* pProp, void* pObject, const ezVariant& value);
  static void RemoveSetPropertyValue(const ezAbstractSetProperty* pProp, void* pObject, const ezVariant& value);

  static ezVariant GetMapPropertyValue(const ezAbstractMapProperty* pProp, const void* pObject, const char* szKey);
  static void SetMapPropertyValue(const ezAbstractMapProperty* pProp, void* pObject, const char* szKey, const ezVariant& value);

  static void InsertArrayPropertyValue(const ezAbstractArrayProperty* pProp, void* pObject, const ezVariant& value, ezUInt32 uiIndex);
  static void RemoveArrayPropertyValue(const ezAbstractArrayProperty* pProp, void* pObject, ezUInt32 uiIndex);

  static const ezAbstractMemberProperty* GetMemberProperty(const ezRTTI* pRtti, ezUInt32 uiPropertyIndex);
  static const ezAbstractMemberProperty* GetMemberProperty(const ezRTTI* pRtti, const char* szPropertyName); // [tested] via ToolsFoundation

  /// \brief Gathers all RTTI types that are derived from pRtti.
  ///
  /// This includes all classes that have pRtti as a base class, either direct or indirect.
  ///
  /// \sa GatherDependentTypes
  static void GatherTypesDerivedFromClass(const ezRTTI* pRtti, ezSet<const ezRTTI*>& out_types);

  /// \brief Gathers all RTTI types that pRtti depends on and adds them to inout_types.
  ///
  /// Dependencies are either member properties or base classes. The output contains the transitive closure of the dependencies.
  /// Note that inout_typesAsSet is not cleared when this function is called.
  /// out_pTypesAsStack is all the dependencies sorted by their appearance in the dependency chain.
  /// The last entry is the lowest in the chain and has no dependencies on its own.
  static void GatherDependentTypes(const ezRTTI* pRtti, ezSet<const ezRTTI*>& inout_typesAsSet, ezDynamicArray<const ezRTTI*>* out_pTypesAsStack = nullptr);

  /// \brief Sorts the input types according to their dependencies.
  ///
  /// Types that have no dependences come first in the output followed by types that have their dependencies met by
  /// the previous entries in the output.
  /// If a dependent type is not in the given types set the function will fail.
  static ezResult CreateDependencySortedTypeArray(const ezSet<const ezRTTI*>& types, ezDynamicArray<const ezRTTI*>& out_sortedTypes);

  struct EnumConversionMode
  {
    enum Enum
    {
      FullyQualifiedName,
      ValueNameOnly,
      Default = FullyQualifiedName
    };

    using StorageType = ezUInt8;
  };

  /// \brief Converts an enum or bitfield value into its string representation.
  ///
  /// The type of pEnumerationRtti will be automatically detected. The syntax of out_sOutput equals MSVC debugger output.
  /// For bitflags, multiple values are combined with the '|' operator (e.g., "Flag1 | Flag2").
  /// For enums, single values are returned (e.g., "MyEnum::Value1").
  ///
  /// \param conversionMode Controls whether to include the full type name or just the value name
  /// \return false if the type is not an enum/bitflag or if the value is invalid
  static bool EnumerationToString(const ezRTTI* pEnumerationRtti, ezInt64 iValue, ezStringBuilder& out_sOutput,
    ezEnum<EnumConversionMode> conversionMode = EnumConversionMode::Default); // [tested]

  /// \brief Helper template to shorten the call for ezEnums
  template <typename T>
  static bool EnumerationToString(ezEnum<T> value, ezStringBuilder& out_sOutput, ezEnum<EnumConversionMode> conversionMode = EnumConversionMode::Default)
  {
    return EnumerationToString(ezGetStaticRTTI<T>(), value.GetValue(), out_sOutput, conversionMode);
  }

  /// \brief Helper template to shorten the call for ezBitflags
  template <typename T>
  static bool BitflagsToString(ezBitflags<T> value, ezStringBuilder& out_sOutput, ezEnum<EnumConversionMode> conversionMode = EnumConversionMode::Default)
  {
    return EnumerationToString(ezGetStaticRTTI<T>(), value.GetValue(), out_sOutput, conversionMode);
  }

  struct EnumKeyValuePair
  {
    ezString m_sKey;
    ezInt32 m_iValue = 0;
  };

  /// \brief If the given type is an enum, \a entries will be filled with all available keys (strings) and values (integers).
  static void GetEnumKeysAndValues(const ezRTTI* pEnumerationRtti, ezDynamicArray<EnumKeyValuePair>& ref_entries, ezEnum<EnumConversionMode> conversionMode = EnumConversionMode::Default);

  /// \brief Converts an enum or bitfield in its string representation to its value.
  ///
  /// The type of pEnumerationRtti will be automatically detected. The syntax of szValue must equal the MSVC debugger output.
  static bool StringToEnumeration(const ezRTTI* pEnumerationRtti, const char* szValue, ezInt64& out_iValue); // [tested]

  /// \brief Helper template to shorten the call for ezEnums
  template <typename T>
  static bool StringToEnumeration(const char* szValue, ezEnum<T>& out_value)
  {
    ezInt64 value;
    const auto retval = StringToEnumeration(ezGetStaticRTTI<T>(), szValue, value);
    out_value = static_cast<typename T::Enum>(value);
    return retval;
  }

  /// \brief Returns the default value (Enum::Default) for the given enumeration type.
  static ezInt64 DefaultEnumerationValue(const ezRTTI* pEnumerationRtti); // [tested]

  /// \brief Makes sure the given value is valid under the given enumeration type.
  ///
  /// Invalid bitflag bits are removed and an invalid enum value is replaced by the default value.
  static ezInt64 MakeEnumerationValid(const ezRTTI* pEnumerationRtti, ezInt64 iValue); // [tested]

  /// \brief Templated convenience function that calls IsEqual and automatically deduces the type.
  template <typename T>
  static bool IsEqual(const T* pObject, const T* pObject2)
  {
    return IsEqual(pObject, pObject2, ezGetStaticRTTI<T>());
  }

  /// \brief Compares pObject with pObject2 of type pType and returns whether they are equal.
  ///
  /// Performs a deep comparison of all reflected properties. For classes derived from ezReflectedClass,
  /// the correct derived type will automatically be determined, so it is not necessary to pass the exact
  /// type into pType. However, the function will return false if pObject and pObject2 actually have
  /// different types.
  ///
  /// Performance warning: This function recursively compares all properties, which can be expensive
  /// for complex object hierarchies or large containers.
  static bool IsEqual(const void* pObject, const void* pObject2, const ezRTTI* pType); // [tested]

  /// \brief Compares property pProp of pObject and pObject2 and returns whether it is equal in both.
  static bool IsEqual(const void* pObject, const void* pObject2, const ezAbstractProperty* pProp);

  /// \brief Deletes pObject using the allocator found in the owning property's type.
  static void DeleteObject(void* pObject, const ezAbstractProperty* pOwnerProperty);

  /// \brief Returns a global default initialization value for the given variant type.
  static ezVariant GetDefaultVariantFromType(ezVariant::Type::Enum type); // [tested]

  /// \brief Returns the default value for the specific type
  static ezVariant GetDefaultVariantFromType(const ezRTTI* pRtti);

  /// \brief Returns the default value for the specific type of the given property.
  static ezVariant GetDefaultValue(const ezAbstractProperty* pProperty, ezVariant index = ezVariant());


  /// \brief Sets all member properties in \a pObject of type \a pRtti to the value returned by ezToolsReflectionUtils::GetDefaultValue()
  static void SetAllMemberPropertiesToDefault(const ezRTTI* pRtti, void* pObject);

  /// \brief If pAttrib is valid and its min/max values are compatible, value will be clamped to them.
  /// Returns false if a clamp attribute exists but no clamp code was executed.
  static ezResult ClampValue(ezVariant& value, const ezClampValueAttribute* pAttrib);
};
