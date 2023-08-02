#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

struct ezScriptBaseClassAttribute_Function;

class ezVisualScriptNodeRegistry
{
  EZ_DECLARE_SINGLETON(ezVisualScriptNodeRegistry);

public:
  struct PinDesc
  {
    ezHashedString m_sName;
    ezHashedString m_sDynamicPinProperty;
    const ezRTTI* m_pDataType = nullptr;
    ezEnum<ezVisualScriptDataType> m_ScriptDataType;
    bool m_bRequired = false;
    bool m_bSplitExecution = false;

    EZ_ALWAYS_INLINE bool IsExecutionPin() const { return m_ScriptDataType == ezVisualScriptDataType::Invalid; }
    EZ_ALWAYS_INLINE bool IsDataPin() const { return m_ScriptDataType != ezVisualScriptDataType::Invalid; }

    static ezColor GetColorForScriptDataType(ezVisualScriptDataType::Enum dataType);
    ezColor GetColor() const;
  };

  struct NodeDesc
  {
    struct TypeDeductionMode
    {
      using StorageType = ezUInt8;

      enum Enum
      {
        None,
        FromInputPins,
        FromTypeProperty,
        FromNameProperty,

        Default = None
      };
    };

    ezSmallArray<PinDesc, 4> m_InputPins;
    ezSmallArray<PinDesc, 4> m_OutputPins;
    ezHashedString m_sFilterByBaseClass;
    const ezRTTI* m_pTargetType = nullptr;
    ezSmallArray<const ezAbstractProperty*, 1> m_TargetProperties;
    ezEnum<ezVisualScriptNodeDescription::Type> m_Type;
    ezEnum<TypeDeductionMode> m_TypeDeductionMode;
    bool m_bImplicitExecution = false;
    bool m_bHasDynamicPins = false;

    void AddInputExecutionPin(ezStringView sName, const ezHashedString& sDynamicPinProperty = ezHashedString());
    void AddOutputExecutionPin(ezStringView sName, const ezHashedString& sDynamicPinProperty = ezHashedString(), bool bSplitExecution = false);

    void AddInputDataPin(ezStringView sName, const ezRTTI* pDataType, ezVisualScriptDataType::Enum scriptDataType, bool bRequired, const ezHashedString& sDynamicPinProperty = ezHashedString());
    void AddOutputDataPin(ezStringView sName, const ezRTTI* pDataType, ezVisualScriptDataType::Enum scriptDataType, const ezHashedString& sDynamicPinProperty = ezHashedString());

    EZ_ALWAYS_INLINE bool NeedsTypeDeduction() const { return m_TypeDeductionMode != TypeDeductionMode::None; }
  };

  ezVisualScriptNodeRegistry();
  ~ezVisualScriptNodeRegistry();

  const ezRTTI* GetNodeBaseType() const { return m_pBaseType; }
  const NodeDesc* GetNodeDescForType(const ezRTTI* pRtti) const { return m_TypeToNodeDescs.GetValue(pRtti); }

  const ezMap<const ezRTTI*, NodeDesc>& GetAllNodeTypes() const { return m_TypeToNodeDescs; }

  static constexpr const char* s_szTypeNamePrefix = "VisualScriptNode_";
  static constexpr ezUInt32 s_uiTypeNamePrefixLength = ezStringUtils::GetStringElementCount(s_szTypeNamePrefix);

private:
  void PhantomTypeRegistryEventHandler(const ezPhantomRttiManagerEvent& e);
  void UpdateNodeTypes();
  void UpdateNodeType(const ezRTTI* pRtti);

  ezResult GetScriptDataType(const ezRTTI* pRtti, ezVisualScriptDataType::Enum& out_scriptDataType, ezStringView sFunctionName = ezStringView(), ezStringView sArgName = ezStringView());
  ezVisualScriptDataType::Enum GetScriptDataType(const ezAbstractProperty* pProp);

  template <typename T>
  void AddInputDataPin(ezReflectedTypeDescriptor& ref_typeDesc, ezVisualScriptNodeRegistry::NodeDesc& ref_nodeDesc, ezStringView sName);
  void AddInputDataPin_Any(ezReflectedTypeDescriptor& ref_typeDesc, ezVisualScriptNodeRegistry::NodeDesc& ref_nodeDesc, ezStringView sName, bool bRequired, bool bAddVariantProperty = false);

  template <typename T>
  void AddOutputDataPin(ezVisualScriptNodeRegistry::NodeDesc& ref_nodeDesc, ezStringView sName);

  void CreateBuiltinTypes();
  void CreateGetOwnerNodeType(const ezRTTI* pRtti);
  void CreateFunctionCallNodeType(const ezRTTI* pRtti, const ezAbstractFunctionProperty* pFunction, const ezScriptableFunctionAttribute* pScriptableFunctionAttribute, bool bIsEntryFunction);
  void CreateCoroutineNodeType(const ezRTTI* pRtti);
  void CreateMessageNodeTypes(const ezRTTI* pRtti);
  void CreateEnumNodeTypes(const ezRTTI* pRtti);

  void FillDesc(ezReflectedTypeDescriptor& desc, const ezRTTI* pRtti, ezStringView sCategoryOverride = ezStringView(), const ezColorGammaUB* pColorOverride = nullptr);
  void FillDesc(ezReflectedTypeDescriptor& desc, ezStringView sTypeName, ezStringView sCategory, const ezColorGammaUB& color);

  const ezRTTI* m_pBaseType = nullptr;
  bool m_bBuiltinTypesCreated = false;
  ezMap<const ezRTTI*, NodeDesc> m_TypeToNodeDescs;
  ezHashSet<const ezRTTI*> m_EnumTypes;
};
