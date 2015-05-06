#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>

class ezMaterialAssetObjectManager : public ezDocumentObjectManagerBase
{
public:
  ezMaterialAssetObjectManager();
  virtual void GetCreateableTypes(ezHybridArray<ezReflectedTypeHandle, 32>& Types) const override;

private:

  virtual ezDocumentObjectBase* InternalCreateObject(ezReflectedTypeHandle hType) override;
  virtual void InternalDestroyObject(ezDocumentObjectBase* pObject) override;
  virtual bool InternalCanAdd(ezReflectedTypeHandle hType, const ezDocumentObjectBase* pParent) const override;
  virtual bool InternalCanRemove(const ezDocumentObjectBase* pObject) const override;
  virtual bool InternalCanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent, ezInt32 iChildIndex) const override;

};


