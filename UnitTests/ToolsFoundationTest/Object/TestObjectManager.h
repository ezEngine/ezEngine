#pragma once

#include <ToolsFoundation/Object/DocumentObjectManager.h>


class ezTestDocumentObjectManager : public ezDocumentObjectManager
{
public:
  ezTestDocumentObjectManager();
  ~ezTestDocumentObjectManager();

  virtual void GetCreateableTypes(ezHybridArray<ezRTTI*, 32>& Types) const override;

  const ezRTTI* m_pMetaRtti;

private:
  virtual ezDocumentObjectBase* InternalCreateObject(const ezRTTI* pRtti) override;
};

