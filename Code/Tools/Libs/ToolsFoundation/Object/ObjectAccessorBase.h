#pragma once

#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezDocumentObject;

class EZ_TOOLSFOUNDATION_DLL ezObjectAccessorBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezObjectAccessorBase, ezReflectedClass);

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

  /// \brief If this accessor is a proxy accessor, transform the input parameters into those of the source accessor. The default implementation does nothing and returns this.
  /// Usually this only needs to be implemented on ezObjectProxyAccessor derived accessors that modify the type, property, view etc of an object.
  /// @param ref_pObject In: proxy object, out: source object.
  /// @param ref_pType In: proxy type, out: source type.
  /// @param ref_pProp In: proxy property, out: source property.
  /// @param ref_indices In: proxy indices, out: source indices. While most of the time this will be one index, e.g. an array or map index. In case of variants that can store containers in containers this can be a chain of indices into a variant hierarchy.
  /// @return Returns the source accessor.
  virtual ezObjectAccessorBase* ResolveProxy(const ezDocumentObject*& ref_pObject, const ezRTTI*& ref_pType, const ezAbstractProperty*& ref_pProp, ezDynamicArray<ezVariant>& ref_indices) { return this; }
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

  const ezAbstractProperty* FindPropertyByName(const ezDocumentObject* pObject, ezStringView sProp);

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
