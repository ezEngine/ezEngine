#pragma once

#include <Foundation/Containers/Set.h>
#include <Foundation/Reflection/Reflection.h>

class ezVariant;

/// \brief Helper functions for handling reflection related operations.
class EZ_FOUNDATION_DLL ezReflectionUtils
{
public:
  static const ezRTTI* GetCommonBaseType(const ezRTTI* pRtti1, const ezRTTI* pRtti2);

  /// \brief Returns whether a type can be stored directly inside a ezVariant.
  static bool IsBasicType(const ezRTTI* pRtti);

  /// \brief Returns the RTTI type matching the variant's type.
  static const ezRTTI* GetTypeFromVariant(const ezVariant& value);
  static const ezRTTI* GetTypeFromVariant(ezVariantType::Enum type);

  /// \brief Sets the Nth component of the vector to the given value.
  ///
  /// vector's type needs to be in between ezVariant::Type::Vector2 and ezVariant::Type::Vector4U.
  static ezUInt32 GetComponentCount(ezVariantType::Enum type);
  static void SetComponent(ezVariant& vector, ezUInt32 iComponent, double fValue); // [tested]
  static double GetComponent(const ezVariant& vector, ezUInt32 iComponent);

  static ezVariant GetMemberPropertyValue(const ezAbstractMemberProperty* pProp, const void* pObject); // [tested] via ToolsFoundation
  static void SetMemberPropertyValue(ezAbstractMemberProperty* pProp, void* pObject,
                                     const ezVariant& value); // [tested] via ToolsFoundation

  static ezVariant GetArrayPropertyValue(const ezAbstractArrayProperty* pProp, const void* pObject, ezUInt32 uiIndex);
  static void SetArrayPropertyValue(ezAbstractArrayProperty* pProp, void* pObject, ezUInt32 uiIndex, const ezVariant& value);

  static void InsertSetPropertyValue(ezAbstractSetProperty* pProp, void* pObject, const ezVariant& value);
  static void RemoveSetPropertyValue(ezAbstractSetProperty* pProp, void* pObject, const ezVariant& value);

  static ezVariant GetMapPropertyValue(const ezAbstractMapProperty* pProp, const void* pObject, const char* szKey);
  static void SetMapPropertyValue(ezAbstractMapProperty* pProp, void* pObject, const char* szKey, const ezVariant& value);

  static void InsertArrayPropertyValue(ezAbstractArrayProperty* pProp, void* pObject, const ezVariant& value, ezUInt32 uiIndex);
  static void RemoveArrayPropertyValue(ezAbstractArrayProperty* pProp, void* pObject, ezUInt32 uiIndex);

  static ezAbstractMemberProperty* GetMemberProperty(const ezRTTI* pRtti, ezUInt32 uiPropertyIndex);
  static ezAbstractMemberProperty* GetMemberProperty(const ezRTTI* pRtti, const char* szPropertyName); // [tested] via ToolsFoundation

  /// \brief Gathers all RTTI types that are derived from pRtti.
  ///
  /// This includes all classes that have pRtti as a base class, either direct or indirect.
  /// If bIncludeDependencies is set to true, the resulting set will also contain all dependent types.
  ///
  /// \sa GatherDependentTypes
  static void GatherTypesDerivedFromClass(const ezRTTI* pRtti, ezSet<const ezRTTI*>& out_types, bool bIncludeDependencies);

  /// \brief Gathers all RTTI types that pRtti depends on and adds them to inout_types.
  ///
  /// Dependencies are either member properties or base classes. The output contains the transitive closure of the dependencies.
  /// Note that inout_types is not cleared when this function is called.
  static void GatherDependentTypes(const ezRTTI* pRtti, ezSet<const ezRTTI*>& inout_types);

  /// \brief Sorts the input types according to their dependencies.
  ///
  /// Types that have no dependences come first in the output followed by types that have their dependencies met by
  /// the previous entries in the output.
  /// If circular dependencies are found the function returns false.
  static bool CreateDependencySortedTypeArray(const ezSet<const ezRTTI*>& types, ezDynamicArray<const ezRTTI*>& out_sortedTypes);

  /// \brief Converts an enum or bitfield value into its string representation.
  ///
  /// The type of pEnumerationRtti will be automatically detected. The syntax of out_sOutput equals MSVC debugger output.
  static bool EnumerationToString(const ezRTTI* pEnumerationRtti, ezInt64 iValue, ezStringBuilder& out_sOutput); // [tested]

  /// \brief Converts an enum or bitfield in its string representation to its value.
  ///
  /// The type of pEnumerationRtti will be automatically detected. The syntax of szValue must equal the MSVC debugger output.
  static bool StringToEnumeration(const ezRTTI* pEnumerationRtti, const char* szValue, ezInt64& out_iValue); // [tested]

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
  /// In case a class derived from ezReflectedClass is passed in the correct derived type
  /// will automatically be determined so it is not necessary to put the exact type into pType,
  /// any derived class type will do. However, the function will return false  pObject and pObject2
  /// actually have a different type.
  static bool IsEqual(const void* pObject, const void* pObject2, const ezRTTI* pType); // [tested]

  /// \brief Compares property pProp of pObject and pObject2 and returns whether it is equal in both.
  static bool IsEqual(const void* pObject, const void* pObject2, ezAbstractProperty* pProp);

  /// \brief Deletes pObject using the allocator found in the owning property's type.
  static void DeleteObject(void* pObject, ezAbstractProperty* pOwnerProperty);

  /// \brief Returns a global default initialization value for the given variant type.
  static ezVariant GetDefaultVariantFromType(ezVariant::Type::Enum type); // [tested]

  /// \brief Returns the default value for the specific type of the given property.
  static ezVariant GetDefaultValue(const ezAbstractProperty* pProperty);

  /// \brief Sets all member properties in \a pObject of type \a pRtti to the value returned by ezToolsReflectionUtils::GetDefaultValue()
  static void SetAllMemberPropertiesToDefault(const ezRTTI* pRtti, void* pObject);
};
