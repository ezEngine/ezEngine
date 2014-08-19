#pragma once

#include <Foundation/Reflection/Reflection.h>

/// \brief Helper functions for handling reflection related operations.
class EZ_FOUNDATION_DLL ezReflectionUtils
{
public:
  static ezVariant GetMemberPropertyValue(const ezAbstractMemberProperty* pProp, const void* pObject); // [tested] via ToolsFoundation 
  static void SetMemberPropertyValue(ezAbstractMemberProperty* pProp, void* pObject, const ezVariant& value); // [tested] via ToolsFoundation 

  static ezAbstractMemberProperty* GetMemberProperty(const ezRTTI* pRtti, ezUInt32 uiPropertyIndex);
  static ezAbstractMemberProperty* GetMemberProperty(const ezRTTI* pRtti, const char* szPropertyName); // [tested] via ToolsFoundation 
};
