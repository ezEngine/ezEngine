#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectTree.h>

class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectManagerBase
{
public:

  virtual ezDocumentObjectBase* CreateObject(ezReflectedTypeHandle hType) = 0;

  bool CanAdd(ezReflectedTypeHandle hType, const ezDocumentObjectBase* pParent) const;
  bool CanRemove(const ezDocumentObjectBase* pObject) const;
  bool CanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent) const;

private:
  virtual bool InternalCanAdd(ezReflectedTypeHandle hType, const ezDocumentObjectBase* pParent) const = 0;
  virtual bool InternalCanRemove(const ezDocumentObjectBase* pObject) const = 0;
  virtual bool InternalCanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent) const = 0;

  ezDocumentObjectTree* m_pObjectTree;
};
