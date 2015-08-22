#pragma once

#include <ToolsFoundation/Object/DocumentObjectManager.h>


class ezTestDocumentObjectManager : public ezDocumentObjectManager
{
public:
  ezTestDocumentObjectManager();
  ~ezTestDocumentObjectManager();

  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const override;

private:
  virtual ezDocumentObjectBase* InternalCreateObject(const ezRTTI* pRtti) override;
};

