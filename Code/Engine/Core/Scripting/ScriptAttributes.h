#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

/// \brief Add this attribute to a class to add script functions to the szTypeName class.
/// This might be necessary if the specified class is not reflected or to separate script functions from the specified class.
class EZ_CORE_DLL ezScriptExtensionAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScriptExtensionAttribute, ezPropertyAttribute);

public:
  ezScriptExtensionAttribute();
  ezScriptExtensionAttribute(ezStringView sTypeName);

  ezStringView GetTypeName() const { return m_sTypeName; }

private:
  ezUntrackedString m_sTypeName;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Add this attribute to a script function to mark it as a base class function.
/// These are functions that can be entry points to visual scripts or over-writable functions in script languages.
class EZ_CORE_DLL ezScriptBaseClassFunctionAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScriptBaseClassFunctionAttribute, ezPropertyAttribute);

public:
  ezScriptBaseClassFunctionAttribute();
  ezScriptBaseClassFunctionAttribute(ezUInt16 uiIndex);

  ezUInt16 GetIndex() const { return m_uiIndex; }

private:
  ezUInt16 m_uiIndex;
};
