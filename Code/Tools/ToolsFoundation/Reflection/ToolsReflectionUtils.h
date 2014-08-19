#pragma once

#include <Foundation/Reflection/ReflectionUtils.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

/// \brief Helper functions for handling reflection related operations.
class EZ_TOOLSFOUNDATION_DLL ezToolsReflectionUtils
{
public:
  /// \brief Returns a global default initialization value for the given variant type.
  static ezVariant GetDefaultVariantFromType(ezVariant::Type::Enum type); // [tested]

  /// \brief Creates a ReflectedTypeDescriptor from an ezRTTI instance that can be serialized and registered at the ezReflectedTypeManager.
  static void GetReflectedTypeDescriptorFromRtti(const ezRTTI* pRtti, ezReflectedTypeDescriptor& out_desc); // [tested]

  /// \brief Returns a member property by a ezPropertyPath. Also changes the ezRTTI and void* to match the properties owner object.
  static ezAbstractMemberProperty* GetMemberPropertyByPath(const ezRTTI*& inout_pRtti, void*& inout_pData, const ezPropertyPath& path);
};
