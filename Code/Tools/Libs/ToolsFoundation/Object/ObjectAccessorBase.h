#pragma once

#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezDocumentObject;

class EZ_TOOLSFOUNDATION_DLL ezObjectAccessorBase
{
public:
  virtual ~ezObjectAccessorBase();
  const ezDocumentObjectManager* GetObjectManager() const;

  /// \name Transaction Operations
  ///@{

  virtual void StartTransaction(ezStringView sDisplayString);
  virtual void CancelTransaction();
  virtual void FinishTransaction();
  virtual void BeginTemporaryCommands(ezStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands = false);
  virtual void CancelTemporaryCommands();
  virtual void FinishTemporaryCommands();

  ///@}
  /// \name Object Access Interface
  ///@{

  virtual const ezDocumentObject* GetObject(const ezUuid& object) = 0;
  virtual ezStatus GetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value, ezVariant index = ezVariant()) = 0;
  virtual ezStatus SetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) = 0;
  virtual ezStatus InsertValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) = 0;
  virtual ezStatus RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant()) = 0;
  virtual ezStatus MoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& oldIndex, const ezVariant& newIndex) = 0;
  virtual ezStatus GetCount(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezInt32& out_iCount) = 0;

  virtual ezStatus AddObject(const ezDocumentObject* pParent, const ezAbstractProperty* pParentProp, const ezVariant& index, const ezRTTI* pType,
    ezUuid& inout_objectGuid) = 0;
  virtual ezStatus RemoveObject(const ezDocumentObject* pObject) = 0;
  virtual ezStatus MoveObject(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const ezAbstractProperty* pParentProp, const ezVariant& index) = 0;

  virtual ezStatus GetKeys(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDynamicArray<ezVariant>& out_keys) = 0;
  virtual ezStatus GetValues(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDynamicArray<ezVariant>& out_values) = 0;

  ///@}
  /// \name Object Access Convenience Functions
  ///@{

  ezStatus GetValueByName(const ezDocumentObject* pObject, ezStringView sProp, ezVariant& out_value, ezVariant index = ezVariant());
  ezStatus SetValueByName(const ezDocumentObject* pObject, ezStringView sProp, const ezVariant& newValue, ezVariant index = ezVariant());
  ezStatus InsertValueByName(const ezDocumentObject* pObject, ezStringView sProp, const ezVariant& newValue, ezVariant index = ezVariant());
  ezStatus RemoveValueByName(const ezDocumentObject* pObject, ezStringView sProp, ezVariant index = ezVariant());
  ezStatus MoveValueByName(const ezDocumentObject* pObject, ezStringView sProp, const ezVariant& oldIndex, const ezVariant& newIndex);
  ezStatus GetCountByName(const ezDocumentObject* pObject, ezStringView sProp, ezInt32& out_iCount);

  ezStatus AddObjectByName(const ezDocumentObject* pParent, ezStringView sParentProp, const ezVariant& index, const ezRTTI* pType, ezUuid& inout_objectGuid);
  ezStatus MoveObjectByName(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, ezStringView sParentProp, const ezVariant& index);

  ezStatus GetKeysByName(const ezDocumentObject* pObject, ezStringView sProp, ezDynamicArray<ezVariant>& out_keys);
  ezStatus GetValuesByName(const ezDocumentObject* pObject, ezStringView sProp, ezDynamicArray<ezVariant>& out_values);
  const ezDocumentObject* GetChildObjectByName(const ezDocumentObject* pObject, ezStringView sProp, ezVariant index);

  ezStatus ClearByName(const ezDocumentObject* pObject, ezStringView sProp);

  template <typename T>
  T Get(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant());
  template <typename T>
  T GetByName(const ezDocumentObject* pObject, ezStringView sProp, ezVariant index = ezVariant());
  ezInt32 GetCount(const ezDocumentObject* pObject, const ezAbstractProperty* pProp);
  ezInt32 GetCountByName(const ezDocumentObject* pObject, ezStringView sProp);

  ///@}

protected:
  ezObjectAccessorBase(const ezDocumentObjectManager* pManager);
  void FireDocumentObjectStructureEvent(const ezDocumentObjectStructureEvent& e);
  void FireDocumentObjectPropertyEvent(const ezDocumentObjectPropertyEvent& e);

protected:
  const ezDocumentObjectManager* m_pConstManager;
};

#include <ToolsFoundation/Object/Implementation/ObjectAccessorBase_inl.h>
