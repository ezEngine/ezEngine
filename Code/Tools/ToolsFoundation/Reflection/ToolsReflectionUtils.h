#pragma once

#include <Foundation/Reflection/ReflectionUtils.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class ezIReflectedTypeAccessor;
class ezDocumentObject;
class ezAbstractObjectGraph;

/// \brief Helper functions for handling reflection related operations.
class EZ_TOOLSFOUNDATION_DLL ezToolsReflectionUtils
{
public:
  /// \brief Returns a global default initialization value for the given variant type.
  static ezVariant GetDefaultVariantFromType(ezVariant::Type::Enum type); // [tested]

  /// \brief Returns the default value for the specific type of the given property.
  static ezVariant GetDefaultValue(const ezAbstractProperty* pProperty);
  /// \brief Returns the default value for the entire property as it is stored on the editor side.
  static ezVariant GetStorageDefault(const ezAbstractProperty* pProperty);

  static bool GetFloatFromVariant(const ezVariant& val, double& out_fValue);
  static bool GetVariantFromFloat(double fValue, ezVariantType::Enum type, ezVariant& out_val);

  /// \brief Creates a ReflectedTypeDescriptor from an ezRTTI instance that can be serialized and registered at the ezPhantomRttiManager.
  static void GetReflectedTypeDescriptorFromRtti(const ezRTTI* pRtti, ezReflectedTypeDescriptor& out_desc); // [tested]
  static void GetMinimalReflectedTypeDescriptorFromRtti(const ezRTTI* pRtti, ezReflectedTypeDescriptor& out_desc);

  static void GatherObjectTypes(const ezDocumentObject* pObject, ezSet<const ezRTTI*>& inout_types);
  static void SerializeTypes(const ezSet<const ezRTTI*>& types, ezAbstractObjectGraph& typesGraph);

  static bool DependencySortTypeDescriptorArray(ezDynamicArray<ezReflectedTypeDescriptor*>& descriptors);

  /// \brief Sets all member properties in \a pObject of type \a pRtti to the value returned by ezToolsReflectionUtils::GetDefaultValue()
  static void SetAllMemberPropertiesToDefault(const ezRTTI* pRtti, void* pObject);
};
