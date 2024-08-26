#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScriptExtensionAttribute, 1, ezRTTIDefaultAllocator<ezScriptExtensionAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("TypeName", m_sTypeName),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezScriptExtensionAttribute::ezScriptExtensionAttribute() = default;
ezScriptExtensionAttribute::ezScriptExtensionAttribute(ezStringView sTypeName)
  : m_sTypeName(sTypeName)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScriptBaseClassFunctionAttribute, 1, ezRTTIDefaultAllocator<ezScriptBaseClassFunctionAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Index", m_uiIndex),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezScriptBaseClassFunctionAttribute::ezScriptBaseClassFunctionAttribute() = default;
ezScriptBaseClassFunctionAttribute::ezScriptBaseClassFunctionAttribute(ezUInt16 uiIndex)
  : m_uiIndex(uiIndex)
{
}


EZ_STATICLINK_FILE(Core, Core_Scripting_Implementation_ScriptAttributes);
