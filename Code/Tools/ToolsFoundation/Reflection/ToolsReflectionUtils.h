#pragma once

#include <Foundation/Reflection/ReflectionUtils.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class ezIReflectedTypeAccessor;
class ezDocumentObject;

/// \brief Helper functions for handling reflection related operations.
class EZ_TOOLSFOUNDATION_DLL ezToolsReflectionUtils
{
public:
  /// \brief Returns a global default initialization value for the given variant type.
  static ezVariant GetDefaultVariantFromType(ezVariant::Type::Enum type); // [tested]

  static ezVariant GetDefaultValue(const ezAbstractProperty* pProperty);

  static bool GetFloatFromVariant(const ezVariant& val, double& out_fValue);
  static bool GetVariantFromFloat(double fValue, ezVariantType::Enum type, ezVariant& out_val);

  /// \brief Creates a ReflectedTypeDescriptor from an ezRTTI instance that can be serialized and registered at the ezPhantomRttiManager.
  static void GetReflectedTypeDescriptorFromRtti(const ezRTTI* pRtti, ezReflectedTypeDescriptor& out_desc); // [tested]

  static void GatherObjectTypes(const ezDocumentObject* pObject, ezSet<const ezRTTI*>& inout_types, bool bOnlyPhantomTypes = true);

  static bool DependencySortTypeDescriptorArray(ezDynamicArray<ezReflectedTypeDescriptor*>& descriptors);

};
