#pragma once
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

class EZ_TOOLSFOUNDATION_DLL ezNodeCommandAccessor : public ezObjectCommandAccessor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezNodeCommandAccessor, ezObjectCommandAccessor);

public:
  ezNodeCommandAccessor(ezCommandHistory* pHistory);
  ~ezNodeCommandAccessor();

  virtual ezStatus SetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) override;

  virtual ezStatus InsertValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) override;
  virtual ezStatus RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant()) override;
  virtual ezStatus MoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& oldIndex, const ezVariant& newIndex) override;

  virtual ezStatus AddObject(const ezDocumentObject* pParent, const ezAbstractProperty* pParentProp, const ezVariant& index, const ezRTTI* pType, ezUuid& inout_objectGuid) override;
  virtual ezStatus RemoveObject(const ezDocumentObject* pObject) override;

private:
  bool IsNode(const ezDocumentObject* pObject) const;
  bool IsDynamicPinProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProp) const;

  struct ConnectionInfo
  {
    const ezDocumentObject* m_pSource = nullptr;
    const ezDocumentObject* m_pTarget = nullptr;
    ezString m_sSourcePin;
    ezString m_sTargetPin;
  };

  ezStatus DisconnectAllPins(const ezDocumentObject* pObject, ezDynamicArray<ConnectionInfo>& out_oldConnections);
  ezStatus TryReconnectAllPins(const ezDocumentObject* pObject, const ezDynamicArray<ConnectionInfo>& oldConnections);
};
