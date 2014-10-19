#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>

class ezTestObjectManager : public ezDocumentObjectManagerBase
{
public:

  virtual ezDocumentObjectBase* CreateObject(ezReflectedTypeHandle hType) override;
  virtual void GetCreateableTypes(ezHybridArray<ezReflectedTypeHandle, 32>& Types) const override;

private:

  virtual bool InternalCanAdd(ezReflectedTypeHandle hType, const ezDocumentObjectBase* pParent) const override;
  virtual bool InternalCanRemove(const ezDocumentObjectBase* pObject) const override;
  virtual bool InternalCanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent) const override;

};


