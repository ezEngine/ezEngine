#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezDocument;

class ezSceneObjectManager : public ezDocumentObjectManager
{
public:
  ezSceneObjectManager();
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const override;

private:
  virtual ezStatus InternalCanAdd(const ezRTTI* pRtti, const ezDocumentObject* pParent, const char* szParentProperty, const ezVariant& index) const override;
  virtual ezStatus InternalCanSelect(const ezDocumentObject* pObject) const override;

};


