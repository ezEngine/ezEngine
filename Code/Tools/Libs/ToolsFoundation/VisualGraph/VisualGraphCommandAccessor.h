#pragma once
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

/// Command accessor for visual graph documents.
///
/// Extends the base command accessor to handle visual graph-specific operations.
/// When node properties change (such as adding or removing dynamic pins), it automatically
/// disconnects and reconnects pins as needed to maintain graph consistency.
/// Removing a node also removes its connections, and removing a connection object disconnects it first.
/// Adding connection objects and changing which pins a connection object connects is refused,
/// use ezNodeCommands for that instead.
class EZ_TOOLSFOUNDATION_DLL ezVisualGraphCommandAccessor : public ezObjectCommandAccessor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualGraphCommandAccessor, ezObjectCommandAccessor);

public:
  ezVisualGraphCommandAccessor(ezCommandHistory* pHistory);
  ~ezVisualGraphCommandAccessor();

  virtual ezStatus SetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) override;

  virtual ezStatus InsertValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index = ezVariant()) override;
  virtual ezStatus RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index = ezVariant()) override;
  virtual ezStatus MoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& oldIndex, const ezVariant& newIndex) override;

  virtual ezStatus AddObject(const ezDocumentObject* pParent, const ezAbstractProperty* pParentProp, const ezVariant& index, const ezRTTI* pType, ezUuid& inout_objectGuid) override;
  virtual ezStatus RemoveObject(const ezDocumentObject* pObject) override;

  bool IsNode(const ezDocumentObject* pObject) const;
  bool IsConnection(const ezDocumentObject* pObject) const;

  /// Whether pProp is one of the properties of a connection object that say which pins it connects.
  ///
  /// These are only written by ezNodeCommands::AddAndConnectCommand, so SetValue() refuses them.
  bool IsConnectionEndpointProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProp) const;
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
