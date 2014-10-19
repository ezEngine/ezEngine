#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Uuid.h>

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

  const ezDocumentObjectBase* GetParent() const { return m_pParent; }
  const ezHybridArray<ezDocumentObjectBase*, 4>& GetChildren() const { return m_Children; }

  const ezUuid& GetGuid() const { return m_Guid; }

private:
  friend class ezDocumentObjectTree;
  friend class ezDocumentObjectManager;

  ezUuid m_Guid;
  ezDocumentObjectBase* m_pParent;
  ezHybridArray<ezDocumentObjectBase*, 4> m_Children;
};
