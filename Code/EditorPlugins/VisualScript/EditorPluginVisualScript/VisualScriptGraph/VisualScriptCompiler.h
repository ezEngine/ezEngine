#pragma once

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>

class ezVisualScriptCompiler
{
public:
  ezVisualScriptCompiler(ezVisualScriptNodeManager& ref_nodeManager);
  ~ezVisualScriptCompiler();

  void InitModule(ezStringView sBaseClassName, ezStringView sScriptClassName);

  ezResult AddFunction(ezStringView sName, const ezDocumentObject* pEntryObject, const ezDocumentObject* pParentObject = nullptr);

  ezResult Compile(ezStringView sDebugAstOutputPath = ezStringView());

  struct CompiledFunction
  {
    ezString m_sName;
    ezEnum<ezVisualScriptNodeDescription::Type> m_Type;
    ezEnum<ezScriptCoroutineCreationMode> m_CoroutineCreationMode;
    ezDynamicArray<ezVisualScriptNodeDescription> m_NodeDescriptions;
    ezVisualScriptDataDescription m_LocalDataDesc;
  };

  struct CompiledModule
  {
    CompiledModule();

    ezResult Serialize(ezStreamWriter& inout_stream) const;

    ezString m_sBaseClassName;
    ezString m_sScriptClassName;
    ezHybridArray<CompiledFunction, 16> m_Functions;

    ezVisualScriptDataDescription m_InstanceDataDesc;
    ezVisualScriptInstanceDataMapping m_InstanceDataMapping;

    ezVisualScriptDataDescription m_ConstantDataDesc;
    ezVisualScriptDataStorage m_ConstantDataStorage;
    ezHashTable<ezVariant, ezUInt32> m_ConstantDataToIndex;
  };

  const CompiledModule& GetCompiledModule() const { return m_Module; }

  using DataOffset = ezVisualScriptDataDescription::DataOffset;
  struct AstNode;

  struct ExecInput
  {
    EZ_DECLARE_POD_TYPE();

    AstNode* m_pSourceNode = nullptr;
    ezUInt32 m_uiSourcePinIndex = 0;
#if EZ_ENABLED(EZ_PLATFORM_64BIT)
    ezUInt32 m_uiPadding = 0;
#endif
  };

  struct ExecOutput
  {
    EZ_DECLARE_POD_TYPE();

    AstNode* m_pTargetNode = nullptr;
  };

  struct DataInput
  {
    EZ_DECLARE_POD_TYPE();

    AstNode* m_pSourceNode = nullptr;
    ezUInt32 m_uiSourcePinIndex = 0;
    DataOffset m_DataOffset;

    EZ_ALWAYS_INLINE bool IsConnected() const { return m_pSourceNode != nullptr; }
    EZ_ALWAYS_INLINE bool IsConnectedAndLocal() const { return IsConnected() && m_DataOffset.GetSource() == DataOffset::Source::Local; }
  };

  struct DataOutput
  {
    EZ_DECLARE_POD_TYPE();

    DataOffset m_DataOffset;

    EZ_ALWAYS_INLINE bool IsValid() const { return m_DataOffset.IsValid(); }
    EZ_ALWAYS_INLINE bool IsValidAndLocal() const { return IsValid() && m_DataOffset.GetSource() == DataOffset::Source::Local; }
  };

  struct AstNode
  {
    const ezDocumentObject* m_pObject = nullptr;

    ezEnum<ezVisualScriptNodeDescription::Type> m_Type;
    ezEnum<ezVisualScriptDataType> m_DeductedDataType;
    bool m_bImplicitExecution = false;

    ezHashedString m_sTargetTypeName;
    ezVariant m_Value;

    ezSmallArray<ExecInput, 2> m_ExecInputs;
    ezSmallArray<ExecOutput, 2> m_ExecOutputs;
    ezSmallArray<DataInput, 7> m_DataInputs;
    ezSmallArray<DataOutput, 4> m_DataOutputs;
  };

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
  static_assert(sizeof(AstNode) == 256);
#endif

private:
  EZ_ALWAYS_INLINE static ezStringView GetNiceTypeName(const ezDocumentObject* pObject)
  {
    return ezVisualScriptNodeManager::GetNiceTypeName(pObject);
  }

  // Ast node creation
  AstNode& CreateAstNode(ezVisualScriptNodeDescription::Type::Enum type, ezVisualScriptDataType::Enum deductedDataType = ezVisualScriptDataType::Invalid, bool bImplicitExecution = false);
  EZ_ALWAYS_INLINE AstNode& CreateAstNode(ezVisualScriptNodeDescription::Type::Enum type, bool bImplicitExecution)
  {
    return CreateAstNode(type, ezVisualScriptDataType::Invalid, bImplicitExecution);
  }

  AstNode& CreateJumpNode(AstNode* pTargetNode);
  AstNode* CreateAstNodeFromObject(const ezDocumentObject* pObject, const ezVisualScriptNodeRegistry::NodeDesc* pNodeDesc, const ezDocumentObject* pEntryObject, bool bImplicitOnly = false);
  DataInput GetOrCreateDefaultPointerNode(const AstNode& node, const ezRTTI* pRtti);

  void MarkAsCoroutine(AstNode* pEntryAstNode);

  // Pins, inputs and outputs
  void AddConstantDataInput(AstNode& node, const ezVariant& value);
  ezResult AddConstantDataInput(AstNode& node, const ezDocumentObject* pObject, const ezVisualScriptPin* pPin, ezVisualScriptDataType::Enum dataType);
  void AddDataInput(AstNode& node, AstNode* pSourceNode, ezUInt32 uiSourcePinIndex, ezVisualScriptDataType::Enum dataType);
  void AddDataOutput(AstNode& node, ezVisualScriptDataType::Enum dataType);
  DataOutput& GetDataOutputFromInput(const DataInput& dataInput);

  void ConnectExecution(AstNode& sourceNode, AstNode& targetNode, ezUInt32 uiSourcePinIndex = ezInvalidIndex);
  void DisconnectExecution(AstNode& sourceNode, AstNode& targetNode, ezUInt32 uiSourcePinIndex);
  void ExecuteBefore(AstNode& node, AstNode& firstNewNode, AstNode& lastNewNode);
  void ExecuteAfter(AstNode& node, AstNode& firstNewNode, AstNode& lastNewNode);
  void ReplaceExecution(AstNode& oldNode, AstNode& newNode);

  DataOffset GetInstanceDataOffset(ezHashedString sName, ezVisualScriptDataType::Enum dataType);

  // Compilation steps
  ezResult BuildInstanceDataMapping();
  AstNode* BuildExecutionFlow(const ezDocumentObject* pEntryObject);

  ezResult BuildDataStack(AstNode* pEntryAstNode, ezDynamicArray<AstNode*>& out_Stack);
  ezResult BuildDataExecutions(AstNode* pEntryAstNode);

  ezResult InsertTypeConversions(AstNode* pEntryAstNode);

  ezResult ReplaceLoop(AstNode* pEntryAstNode);
  ezResult ReplaceUnsupportedNodes(AstNode* pEntryAstNode);

  ezResult AssignInstanceVariables(AstNode* pEntryAstNode);
  ezResult AssignLocalVariables(AstNode* pEntryAstNode, ezVisualScriptDataDescription& inout_localDataDesc);
  ezResult CopyOutputsToInputs(AstNode* pEntryAstNode);

  ezResult BuildNodeDescriptions(AstNode* pEntryAstNode, ezDynamicArray<ezVisualScriptNodeDescription>& out_NodeDescriptions);

  ezResult FinalizeConstantData();

  enum class VisitorResult
  {
    Continue,
    Skip,
    Error,
  };

  // Does allow modifications to the AST structure while iterating
  ezResult TraverseAstDepthFirst(AstNode* pEntryAstNode, ezDelegate<VisitorResult(AstNode*& pAstNode)> func);

  // Does NOT allow modifications to the AST structure while iterating
  ezResult TraverseAstTopologicalOrder(const AstNode* pEntryAstNode, ezDelegate<VisitorResult(const AstNode* pAstNode)> func);

  void DumpAST(AstNode* pEntryAstNode, ezStringView sOutputPath, ezStringView sFunctionName, ezStringView sSuffix);
  void DumpGraph(ezArrayPtr<const ezVisualScriptNodeDescription> nodeDescriptions, ezStringView sOutputPath, ezStringView sFunctionName, ezStringView sSuffix);

  ezVisualScriptNodeManager& m_NodeManager;

  ezDeque<AstNode> m_AstNodes;
  ezHybridArray<const ezDocumentObject*, 8> m_EntryObjects;

  struct LiveLocalVar
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiId = ezInvalidIndex;
    DataOffset m_DataOffset;

    ezUInt32 m_uiStart = ezInvalidIndex;
    ezUInt32 m_uiEnd = 0;
  };

  struct CompilationState
  {
    ezHashTable<const ezDocumentObject*, AstNode*> m_ExecObjectToAstNode;
    ezHashTable<const ezDocumentObject*, AstNode*> m_DataObjectToAstNode;

    ezHashSet<const AstNode*> m_VisitedNodes;

    ezDynamicArray<LiveLocalVar> m_LiveLocalVars;
    ezUInt32 m_uiNextLocalVarId = 0;

    AstNode* m_pGetScriptOwnerNode = nullptr;

    void Clear()
    {
      m_ExecObjectToAstNode.Clear();
      m_DataObjectToAstNode.Clear();

      m_VisitedNodes.Clear();

      m_LiveLocalVars.Clear();
      m_uiNextLocalVarId = 0;

      m_pGetScriptOwnerNode = nullptr;
    }
  };

  CompilationState m_CompilationState;

  CompiledModule m_Module;
};
