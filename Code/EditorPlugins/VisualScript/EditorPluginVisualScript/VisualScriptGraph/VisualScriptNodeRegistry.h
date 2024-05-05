#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

struct ezNodePropertyValue;
class ezVisualScriptPin;

class ezVisualScriptNodeRegistry
{
  EZ_DECLARE_SINGLETON(ezVisualScriptNodeRegistry);

public:
  struct PinDesc
  {
    ezHashedString m_sName;
    ezHashedString m_sDynamicPinProperty;
    const ezRTTI* m_pDataType = nullptr;

    using DeductTypeFunc = ezVisualScriptDataType::Enum (*)(const ezVisualScriptPin& pin);
    DeductTypeFunc m_DeductTypeFunc = nullptr;

    ezEnum<ezVisualScriptDataType> m_ScriptDataType;
    bool m_bRequired = false;
    bool m_bSplitExecution = false;
    bool m_bReplaceWithArray = false;

    EZ_ALWAYS_INLINE bool IsExecutionPin() const { return m_ScriptDataType == ezVisualScriptDataType::Invalid; }
    EZ_ALWAYS_INLINE bool IsDataPin() const { return m_ScriptDataType != ezVisualScriptDataType::Invalid; }

    static ezColor GetColorForScriptDataType(ezVisualScriptDataType::Enum dataType);
    ezColor GetColor() const;
  };

  struct NodeDesc
  {
    ezSmallArray<PinDesc, 4> m_InputPins;
    ezSmallArray<PinDesc, 4> m_OutputPins;
    ezHashedString m_sFilterByBaseClass;
    const ezRTTI* m_pTargetType = nullptr;
    ezSmallArray<const ezAbstractProperty*, 1> m_TargetProperties;

    using DeductTypeFunc = ezVisualScriptDataType::Enum (*)(const ezDocumentObject* pObject, const ezVisualScriptPin* pDisconnectedPin);
    DeductTypeFunc m_DeductTypeFunc = nullptr;

    ezEnum<ezVisualScriptNodeDescription::Type> m_Type;
    bool m_bImplicitExecution = true;
    bool m_bHasDynamicPins = false;

    void AddInputExecutionPin(ezStringView sName, const ezHashedString& sDynamicPinProperty = ezHashedString());
    void AddOutputExecutionPin(ezStringView sName, const ezHashedString& sDynamicPinProperty = ezHashedString(), bool bSplitExecution = false);

    void AddInputDataPin(ezStringView sName, const ezRTTI* pDataType, ezVisualScriptDataType::Enum scriptDataType, bool bRequired, const ezHashedString& sDynamicPinProperty = ezHashedString(), PinDesc::DeductTypeFunc deductTypeFunc = nullptr, bool bReplaceWithArray = false);
    void AddOutputDataPin(ezStringView sName, const ezRTTI* pDataType, ezVisualScriptDataType::Enum scriptDataType, const ezHashedString& sDynamicPinProperty = ezHashedString(), PinDesc::DeductTypeFunc deductTypeFunc = nullptr);

    EZ_ALWAYS_INLINE bool NeedsTypeDeduction() const { return m_DeductTypeFunc != nullptr; }
  };

  ezVisualScriptNodeRegistry();
  ~ezVisualScriptNodeRegistry();

  const ezRTTI* GetNodeBaseType() const { return m_pBaseType; }
  const ezRTTI* GetVariableSetterType() const { return m_pSetVariableType; }
  const ezRTTI* GetVariableGetterType() const { return m_pGetVariableType; }
  const NodeDesc* GetNodeDescForType(const ezRTTI* pRtti) const { return m_TypeToNodeDescs.GetValue(pRtti); }

  struct NodeCreationTemplate
  {
    const ezRTTI* m_pType = nullptr;
    ezStringView m_sTypeName;
    ezHashedString m_sCategory;
    ezUInt32 m_uiPropertyValuesStart;
    ezUInt32 m_uiPropertyValuesCount;
  };

  const ezArrayPtr<const NodeCreationTemplate> GetNodeCreationTemplates() const { return m_NodeCreationTemplates; }
  const ezArrayPtr<const ezNodePropertyValue> GetPropertyValues() const { return m_PropertyValues; }

  static constexpr const char* s_szTypeNamePrefix = "VisualScriptNode_";
  static constexpr ezUInt32 s_uiTypeNamePrefixLength = ezStringUtils::GetStringElementCount(s_szTypeNamePrefix);

private:
  void PhantomTypeRegistryEventHandler(const ezPhantomRttiManagerEvent& e);
  void UpdateNodeTypes();
  void UpdateNodeType(const ezRTTI* pRtti, bool bForceExpose = false);

  ezResult GetScriptDataType(const ezRTTI* pRtti, ezVisualScriptDataType::Enum& out_scriptDataType, ezStringView sFunctionName = ezStringView(), ezStringView sArgName = ezStringView());
  ezVisualScriptDataType::Enum GetScriptDataType(const ezAbstractProperty* pProp);

  template <typename T>
  void AddInputDataPin(ezReflectedTypeDescriptor& ref_typeDesc, NodeDesc& ref_nodeDesc, ezStringView sName);
  void AddInputDataPin_Any(ezReflectedTypeDescriptor& ref_typeDesc, NodeDesc& ref_nodeDesc, ezStringView sName, bool bRequired, bool bAddVariantProperty = false, PinDesc::DeductTypeFunc deductTypeFunc = nullptr);

  template <typename T>
  void AddOutputDataPin(NodeDesc& ref_nodeDesc, ezStringView sName);

  void CreateBuiltinTypes();
  void CreateGetOwnerNodeType(const ezRTTI* pRtti);
  void CreateFunctionCallNodeType(const ezRTTI* pRtti, const ezHashedString& sCategory, const ezAbstractFunctionProperty* pFunction, const ezScriptableFunctionAttribute* pScriptableFunctionAttribute, bool bIsEntryFunction);
  void CreateCoroutineNodeType(const ezRTTI* pRtti);
  void CreateMessageNodeTypes(const ezRTTI* pRtti);
  void CreateEnumNodeTypes(const ezRTTI* pRtti);

  void FillDesc(ezReflectedTypeDescriptor& desc, const ezRTTI* pRtti, const ezColorGammaUB* pColorOverride = nullptr);
  void FillDesc(ezReflectedTypeDescriptor& desc, ezStringView sTypeName, const ezColorGammaUB& color);

  const ezRTTI* RegisterNodeType(ezReflectedTypeDescriptor& typeDesc, NodeDesc&& nodeDesc, const ezHashedString& sCategory);

  const ezRTTI* m_pBaseType = nullptr;
  const ezRTTI* m_pSetPropertyType = nullptr;
  const ezRTTI* m_pGetPropertyType = nullptr;
  const ezRTTI* m_pSetVariableType = nullptr;
  const ezRTTI* m_pGetVariableType = nullptr;
  bool m_bBuiltinTypesCreated = false;
  ezMap<const ezRTTI*, NodeDesc> m_TypeToNodeDescs;
  ezHashSet<const ezRTTI*> m_ExposedTypes;

  ezDynamicArray<NodeCreationTemplate> m_NodeCreationTemplates;
  ezDynamicArray<ezNodePropertyValue> m_PropertyValues;
  ezDeque<ezString> m_PropertyNodeTypeNames;
};
