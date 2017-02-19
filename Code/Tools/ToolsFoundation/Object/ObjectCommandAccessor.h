#pragma once

#include <ToolsFoundation/Object/ObjectDirectAccessor.h>

class ezDocumentObject;
class ezCommandHistory;

class EZ_TOOLSFOUNDATION_DLL ezObjectCommandAccessor : public ezObjectDirectAccessor
{
public:
  ezObjectCommandAccessor(ezCommandHistory* pHistory);

  virtual void StartTransaction(const char* szDisplayString) override;
  virtual void CancelTransaction() override;
  virtual void FinishTransaction() override;
  virtual void BeginTemporaryCommands(const char* szDisplayString, bool bFireEventsWhenUndoingTempCommands = false) override;
  virtual void CancelTemporaryCommands() override;
  virtual void FinishTemporaryCommands() override;

  virtual ezStatus SetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) override;
  virtual ezStatus InsertValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) override;
  virtual ezStatus RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant()) override;
  virtual ezStatus MoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& oldIndex, const ezVariant& newIndex) override;

  virtual ezStatus AddObject(const ezDocumentObject* pParent, const ezAbstractProperty* pParentProp, const ezVariant& index, const ezRTTI* pType, ezUuid& inout_objectGuid) override;
  virtual ezStatus RemoveObject(const ezDocumentObject* pObject) override;
  virtual ezStatus MoveObject(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const ezAbstractProperty* pParentProp, const ezVariant& index) override;

private:
  ezCommandHistory* m_pHistory;
};
