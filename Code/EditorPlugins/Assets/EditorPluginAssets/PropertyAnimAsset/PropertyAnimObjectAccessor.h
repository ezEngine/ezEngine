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
  typedef ezDelegate<void (const ezUuid&)> OnAddTrack;
  ezUuid FindOrAddTrack(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezPropertyAnimTarget::Enum target, OnAddTrack onAddTrack);

  ezStatus SetCurveCp(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezPropertyAnimTarget::Enum target, double fOldValue, double fNewValue);
  ezStatus SetOrInsertCurveCp(const ezUuid& track, double fValue);

  ezStatus SetColorCurveCp(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, const ezColorGammaUB& oldValue, const ezColorGammaUB& newValue);
  ezStatus SetOrInsertColorCurveCp(const ezUuid& track, const ezColorGammaUB& value);

  ezStatus SetAlphaCurveCp(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezUInt8 oldValue, ezUInt8 newValue);
  ezStatus SetOrInsertAlphaCurveCp(const ezUuid& track, ezUInt8 value);

  ezStatus SetIntensityCurveCp(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, float oldValue, float newValue);
  ezStatus SetOrInsertIntensityCurveCp(const ezUuid& track, float value);

  void SeparateColor(const ezColor& color, ezColorGammaUB& gamma, ezUInt8& alpha, float& intensity);

  ezUniquePtr<ezObjectAccessorBase> m_ObjAccessor;
  ezPropertyAnimAssetDocument* m_pDocument = nullptr;
  ezPropertyAnimObjectManager* m_pObjectManager = nullptr;
};
