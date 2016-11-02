#pragma once

#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezDocumentObject;

class EZ_TOOLSFOUNDATION_DLL ezObjectAccessorBase
{
public:
  /// \name Transaction Operations
  ///@{

  virtual void StartTransaction(const char* szDisplayString);
  virtual void CancelTransaction();
  virtual void FinishTransaction();
  virtual void BeginTemporaryCommands(const char* szDisplayString, bool bFireEventsWhenUndoingTempCommands = false);
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

  virtual ezStatus AddObject(const ezDocumentObject* pParent, const ezAbstractProperty* pParentProp, const ezVariant& index, const ezRTTI* pType, ezUuid& inout_objectGuid) = 0;
  virtual ezStatus RemoveObject(const ezDocumentObject* pObject) = 0;
  virtual ezStatus MoveObject(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const ezAbstractProperty* pParentProp, const ezVariant& index) = 0;
 
  virtual ezStatus GetKeys(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezHybridArray<ezVariant, 16>& out_keys) = 0;
  virtual ezStatus GetValues(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezHybridArray<ezVariant, 16>& out_values) = 0;

  ///@}
  /// \name Object Access Convenience Functions
  ///@{

  ezStatus GetValue(const ezDocumentObject* pObject, const char* szProp, ezVariant& out_value, ezVariant index = ezVariant());
  ezStatus SetValue(const ezDocumentObject* pObject, const char* szProp, const ezVariant& newValue, ezVariant index = ezVariant());
  ezStatus InsertValue(const ezDocumentObject* pObject, const char* szProp, const ezVariant& newValue, ezVariant index = ezVariant());
  ezStatus RemoveValue(const ezDocumentObject* pObject, const char* szProp, ezVariant index = ezVariant());
  ezStatus MoveValue(const ezDocumentObject* pObject, const char* szProp, const ezVariant& oldIndex, const ezVariant& newIndex);
  ezStatus GetCount(const ezDocumentObject* pObject, const char* szProp, ezInt32& out_iCount);

  ezStatus AddObject(const ezDocumentObject* pParent, const char* szParentProp, const ezVariant& index, const ezRTTI* pType, ezUuid& inout_objectGuid);
  ezStatus MoveObject(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const char* szParentProp, const ezVariant& index);

  ezStatus GetKeys(const ezDocumentObject* pObject, const char* szProp, ezHybridArray<ezVariant, 16>& out_keys);
  ezStatus GetValues(const ezDocumentObject* pObject, const char* szProp, ezHybridArray<ezVariant, 16>& out_values);

  template<typename T>
  T Get(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant());
  EZ_FORCE_INLINE ezInt32 GetCount(const ezDocumentObject* pObject, const ezAbstractProperty* pProp);

  ///@}

protected:
  ezObjectAccessorBase(ezDocumentObjectManager* pManager);
  void FireDocumentObjectStructureEvent(const ezDocumentObjectStructureEvent& e);
  void FireDocumentObjectPropertyEvent(const ezDocumentObjectPropertyEvent& e);

protected:
  ezDocumentObjectManager* m_pManager;
};

#include <ToolsFoundation/Object/Implementation/ObjectAccessorBase_inl.h>
