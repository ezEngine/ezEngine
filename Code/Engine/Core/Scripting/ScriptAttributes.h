#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class EZ_CORE_DLL ezScriptExtensionAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScriptExtensionAttribute, ezPropertyAttribute);

public:
  ezScriptExtensionAttribute();
  ezScriptExtensionAttribute(const char* szTypeName);

  const char* GetTypeName() const { return m_sTypeName; }

private:
  ezUntrackedString m_sTypeName;
};

//////////////////////////////////////////////////////////////////////////

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
