#pragma once
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

class EZ_TOOLSFOUNDATION_DLL ezNodeCommandAccessor : public ezObjectCommandAccessor
{
public:
  ezNodeCommandAccessor(ezCommandHistory* pHistory);
  ~ezNodeCommandAccessor();

  virtual ezStatus SetValue(
    const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) override;

  virtual ezStatus InsertValue(
    const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) override;
  virtual ezStatus RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant()) override;
  virtual ezStatus MoveValue(
    const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& oldIndex, const ezVariant& newIndex) override;

private:
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
