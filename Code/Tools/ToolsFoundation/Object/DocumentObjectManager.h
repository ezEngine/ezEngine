#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

class ezDocumentObjectTree;

class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectManagerBase
{
public:
  ezDocumentObjectManagerBase();
  virtual ~ezDocumentObjectManagerBase() { }

  void SetObjectTree(const ezDocumentObjectTree* pDocumentTree);
  ezDocumentObjectBase* CreateObject(const ezRTTI* pRtti, ezUuid guid = ezUuid());
  void DestroyObject(ezDocumentObjectBase* pObject);

  virtual void GetCreateableTypes(ezHybridArray<ezRTTI*, 32>& Types) const = 0;

  bool CanAdd(const ezRTTI* pRtti, const ezDocumentObjectBase* pParent) const;
  bool CanRemove(const ezDocumentObjectBase* pObject) const;
  bool CanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent, ezInt32 iChildIndex = -1) const;

private:
  virtual ezDocumentObjectBase* InternalCreateObject(const ezRTTI* pRtti) = 0;
  virtual void InternalDestroyObject(ezDocumentObjectBase* pObject) = 0;
  virtual bool InternalCanAdd(const ezRTTI* pRtti, const ezDocumentObjectBase* pParent) const = 0;
  virtual bool InternalCanRemove(const ezDocumentObjectBase* pObject) const = 0;
  virtual bool InternalCanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent, ezInt32 iChildIndex) const = 0;

  const ezDocumentObjectTree* m_pDocumentTree;
};
