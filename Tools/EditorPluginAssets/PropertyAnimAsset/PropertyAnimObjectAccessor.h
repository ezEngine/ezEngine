#pragma once
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

class ezPropertyAnimAssetDocument;
class ezPropertyAnimObjectManager;

class ezPropertyAnimObjectAccessor : public ezObjectCommandAccessor
{
public:
  ezPropertyAnimObjectAccessor(ezPropertyAnimAssetDocument* pDoc, ezCommandHistory* pHistory);

  virtual ezStatus GetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value, ezVariant index = ezVariant()) override;
  virtual ezStatus SetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) override;

  virtual ezStatus InsertValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) override;
  virtual ezStatus RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant()) override;
  virtual ezStatus MoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& oldIndex, const ezVariant& newIndex) override;

  virtual ezStatus AddObject(const ezDocumentObject* pParent, const ezAbstractProperty* pParentProp, const ezVariant& index, const ezRTTI* pType, ezUuid& inout_objectGuid) override;
  virtual ezStatus RemoveObject(const ezDocumentObject* pObject) override;
  virtual ezStatus MoveObject(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const ezAbstractProperty* pParentProp, const ezVariant& index) override;

private:
  bool IsTemporary(const ezDocumentObject* pObject) const;
  bool IsTemporary(const ezDocumentObject* pParent, const ezAbstractProperty* pParentProp) const;


  ezObjectCommandAccessor m_ObjAccessor;
  ezPropertyAnimAssetDocument* m_pDocument = nullptr;
  ezPropertyAnimObjectManager* m_pObjectManager = nullptr;
};
