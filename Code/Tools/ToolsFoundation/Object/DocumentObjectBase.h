#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <Foundation/Strings/HashedString.h>

class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectBaseProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentObjectBaseProperties);

public:

  void SetName(const char* szName) { m_sName.Assign(szName); }
  const char* GetName() const { return m_sName.GetString().GetData(); }

protected:
  ezHashedString m_sName;
};

class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectBase
{
public:

  virtual const ezIReflectedTypeAccessor& GetTypeAccessor() const = 0;
  virtual const ezIReflectedTypeAccessor& GetEditorTypeAccessor() const = 0;

  // void GetParent, GetChildren

private:

};
