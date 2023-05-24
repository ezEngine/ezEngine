#pragma once

#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezPropertyAnimObjectManager : public ezDocumentObjectManager
{
public:
  ezPropertyAnimObjectManager();
  ~ezPropertyAnimObjectManager();

  bool GetAllowStructureChangeOnTemporaries() const { return m_bAllowStructureChangeOnTemporaries; }
  void SetAllowStructureChangeOnTemporaries(bool bVal) { m_bAllowStructureChangeOnTemporaries = bVal; }

private:
  virtual ezStatus InternalCanAdd(
    const ezRTTI* pRtti, const ezDocumentObject* pParent, const char* szParentProperty, const ezVariant& index) const override;
  virtual ezStatus InternalCanRemove(const ezDocumentObject* pObject) const override;
  virtual ezStatus InternalCanMove(
    const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const char* szParentProperty, const ezVariant& index) const override;

private:
  bool m_bAllowStructureChangeOnTemporaries = false;
};
