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

  static void GetPropertyPathFromString(const char* szPath, ezPropertyPath& out_Path, ezHybridArray<ezString, 6>& out_Storage);

  static ezString GetStringFromPropertyPath(const ezPropertyPath& Path);

  static ezPropertyPath CreatePropertyPath(const char* pData1, const char* pData2 = nullptr, const char* pData3 = nullptr, const char* pData4 = nullptr, const char* pData5 = nullptr, const char* pData6 = nullptr);

  /// \brief Returns a member property by a ezPropertyPath. Also changes the ezRTTI and void* to match the properties owner object.
  static ezAbstractMemberProperty* GetMemberPropertyByPath(const ezRTTI*& inout_pRtti, void*& inout_pData, const ezPropertyPath& path);
};
