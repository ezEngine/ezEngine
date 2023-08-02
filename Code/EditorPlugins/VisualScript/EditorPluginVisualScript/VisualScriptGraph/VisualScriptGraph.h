#pragma once

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptNodeRegistry.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class ezVisualScriptPin : public ezPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptPin, ezPin);

public:
  ezVisualScriptPin(Type type, ezStringView sName, const ezVisualScriptNodeRegistry::PinDesc& pinDesc, const ezDocumentObject* pObject, ezUInt32 uiDataPinIndex);

  EZ_ALWAYS_INLINE bool IsExecutionPin() const { return m_ScriptDataType == ezVisualScriptDataType::Invalid; }
  EZ_ALWAYS_INLINE bool IsDataPin() const { return m_ScriptDataType != ezVisualScriptDataType::Invalid; }

  EZ_ALWAYS_INLINE const ezRTTI* GetDataType() const { return m_pDataType; }
  EZ_ALWAYS_INLINE ezVisualScriptDataType::Enum GetScriptDataType() const { return m_ScriptDataType; }
  ezStringView GetDataTypeName(ezVisualScriptDataType::Enum deductedType = ezVisualScriptDataType::Invalid) const;
  EZ_ALWAYS_INLINE ezUInt32 GetDataPinIndex() const { return m_uiDataPinIndex; }
  EZ_ALWAYS_INLINE bool IsRequired() const { return m_bRequired; }
  EZ_ALWAYS_INLINE bool HasDynamicPinProperty() const { return m_bHasDynamicPinProperty; }
  EZ_ALWAYS_INLINE bool SplitExecution() const { return m_bSplitExecution; }

  bool CanConvertTo(const ezVisualScriptPin& targetPin, ezVisualScriptDataType::Enum deductedSourceDataType = ezVisualScriptDataType::Invalid, ezVisualScriptDataType::Enum deductedTargetDataType = ezVisualScriptDataType::Invalid) const;

private:
  const ezRTTI* m_pDataType = nullptr;
  ezUInt32 m_uiDataPinIndex = 0;
  ezEnum<ezVisualScriptDataType> m_ScriptDataType;
  bool m_bRequired = false;
  bool m_bHasDynamicPinProperty = false;
  bool m_bSplitExecution = false;
};

class ezVisualScriptNodeManager : public ezDocumentNodeManager
{
public:
  ezVisualScriptNodeManager();
  ~ezVisualScriptNodeManager();

  ezHashedString GetScriptBaseClass() const;
  bool IsFilteredByBaseClass(const ezRTTI* pNodeType, const ezVisualScriptNodeRegistry::NodeDesc& nodeDesc, const ezHashedString& sBaseClass, bool bLogWarning = false) const;

  ezVisualScriptDataType::Enum GetVariableType(ezTempHashedString sName) const;
  ezResult GetVariableDefaultValue(ezTempHashedString sName, ezVariant& out_Value) const;

  void GetInputExecutionPins(const ezDocumentObject* pObject, ezDynamicArray<const ezVisualScriptPin*>& out_pins) const;
  void GetOutputExecutionPins(const ezDocumentObject* pObject, ezDynamicArray<const ezVisualScriptPin*>& out_pins) const;

  void GetInputDataPins(const ezDocumentObject* pObject, ezDynamicArray<const ezVisualScriptPin*>& out_pins) const;
  void GetOutputDataPins(const ezDocumentObject* pObject, ezDynamicArray<const ezVisualScriptPin*>& out_pins) const;

  void GetEntryNodes(const ezDocumentObject* pObject, ezDynamicArray<const ezDocumentObject*>& out_entryNodes) const;

  static ezStringView GetNiceTypeName(const ezDocumentObject* pObject);
  static ezStringView GetNiceFunctionName(const ezDocumentObject* pObject);

  ezVisualScriptDataType::Enum GetDeductedType(const ezVisualScriptPin& pin) const;
  ezVisualScriptDataType::Enum GetDeductedType(const ezDocumentObject* pObject) const;

  bool IsCoroutine(const ezDocumentObject* pObject) const;

  ezEvent<const ezDocumentObject*> m_NodeChangedEvent;

private:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const override;
  virtual bool InternalIsDynamicPinProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProp) const override;
  virtual ezStatus InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_Result) const override;

  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node) override;

  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const override;

  void NodeEventsHandler(const ezDocumentNodeManagerEvent& e);
  void PropertyEventsHandler(const ezDocumentObjectPropertyEvent& e);

  void DeductType(const ezDocumentObject* pObject, const ezPin* pChangedPin = nullptr, bool bConnected = true);
  void UpdateCoroutine(const ezDocumentObject* pTargetNode, const ezConnection& changedConnection, bool bConnected);
  bool IsConnectedToCoroutine(const ezDocumentObject* pEntryNode, const ezConnection& changedConnection, bool bConnected) const;

  ezHashTable<const ezDocumentObject*, ezEnum<ezVisualScriptDataType>> m_ObjectToDeductedType;
  ezHashSet<const ezDocumentObject*> m_CoroutineObjects;
};
