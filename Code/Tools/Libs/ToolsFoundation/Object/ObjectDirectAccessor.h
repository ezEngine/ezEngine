#pragma once

#include <ToolsFoundation/Object/ObjectAccessorBase.h>

class ezDocumentObjectManager;

class EZ_TOOLSFOUNDATION_DLL ezObjectDirectAccessor : public ezObjectAccessorBase
{
public:
  ezObjectDirectAccessor(ezDocumentObjectManager* pManager);

  virtual const ezDocumentObject* GetObject(const ezUuid& object) override;
  virtual ezStatus GetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value, ezVariant index = ezVariant()) override;
  virtual ezStatus SetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) override;
  virtual ezStatus InsertValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) override;
  virtual ezStatus RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant()) override;
  virtual ezStatus MoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& oldIndex, const ezVariant& newIndex) override;
  virtual ezStatus GetCount(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezInt32& out_iCount) override;

  virtual ezStatus AddObject(const ezDocumentObject* pParent, const ezAbstractProperty* pParentProp, const ezVariant& index, const ezRTTI* pType, ezUuid& inout_objectGuid) override;
  virtual ezStatus RemoveObject(const ezDocumentObject* pObject) override;
  virtual ezStatus MoveObject(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const ezAbstractProperty* pParentProp, const ezVariant& index) override;

  virtual ezStatus GetKeys(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezHybridArray<ezVariant, 16>& out_keys) override;
  virtual ezStatus GetValues(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezHybridArray<ezVariant, 16>& out_values) override;

protected:
  ezDocumentObjectManager* m_pManager;

};

