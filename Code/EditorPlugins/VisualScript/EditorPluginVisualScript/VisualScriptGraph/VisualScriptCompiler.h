#pragma once

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>

class ezVisualScriptCompiler
{
public:
  ezVisualScriptCompiler();
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
  };

  const CompiledModule& GetCompiledModule() const { return m_Module; }

  struct AstNode;

  struct DataInput
  {
    EZ_DECLARE_POD_TYPE();

    AstNode* m_pSourceNode = nullptr;
    ezUInt32 m_uiId = 0;
    ezUInt8 m_uiSourcePinIndex = 0;
    ezEnum<ezVisualScriptDataType> m_DataType;
  };

  struct DataOutput
  {
    ezSmallArray<AstNode*, 3> m_TargetNodes;
    ezUInt32 m_uiId = 0;
    ezEnum<ezVisualScriptDataType> m_DataType;
  };

  struct AstNode
  {
    ezEnum<ezVisualScriptNodeDescription::Type> m_Type;
    ezEnum<ezVisualScriptDataType> m_DeductedDataType;
    bool m_bImplicitExecution = false;

    ezHashedString m_sTargetTypeName;
    ezVariant m_Value;

    ezSmallArray<AstNode*, 4> m_Next;
    ezSmallArray<DataInput, 5> m_Inputs;
    ezSmallArray<DataOutput, 2> m_Outputs;
  };

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
  static_assert(sizeof(AstNode) == 256);
#endif

private:
  using DataOffset = ezVisualScriptDataDescription::DataOffset;

  EZ_ALWAYS_INLINE static ezStringView GetNiceTypeName(const ezDocumentObject* pObject)
  {
    return ezVisualScriptNodeManager::GetNiceTypeName(pObject);
  }

  EZ_ALWAYS_INLINE ezVisualScriptDataType::Enum GetDeductedType(const ezDocumentObject* pObject) const
  {
    return m_pManager->GetDeductedType(pObject);
  }

  ezUInt32 GetPinId(const ezVisualScriptPin* pPin);
  DataOutput& GetDataOutput(const DataInput& dataInput);
  AstNode& CreateAstNode(ezVisualScriptNodeDescription::Type::Enum type, ezVisualScriptDataType::Enum deductedDataType = ezVisualScriptDataType::Invalid, bool bImplicitExecution = false);
  EZ_ALWAYS_INLINE AstNode& CreateAstNode(ezVisualScriptNodeDescription::Type::Enum type, bool bImplicitExecution)
  {
    return CreateAstNode(type, ezVisualScriptDataType::Invalid, bImplicitExecution);
  }

  void AddDataInput(AstNode& node, AstNode* pSourceNode, ezUInt8 uiSourcePinIndex, ezVisualScriptDataType::Enum dataType);
  void AddDataOutput(AstNode& node, ezVisualScriptDataType::Enum dataType);

  struct DefaultInput
  {
    AstNode* m_pSourceNode = nullptr;
    ezUInt8 m_uiSourcePinIndex = 0;
  };

  DefaultInput GetDefaultPointerInput(const ezRTTI* pDataType);
  AstNode* CreateConstantNode(const ezVariant& value);
  AstNode* CreateJumpNode(AstNode* pTargetNode);

  DataOffset GetInstanceDataOffset(ezHashedString sName, ezVisualScriptDataType::Enum dataType);

  struct ConnectionType
  {
    enum Enum
    {
      Execution,
      Data,
    };
  };

  struct Connection
  {
    AstNode* m_pSource = nullptr;
    AstNode* m_pTarget = nullptr;
    ConnectionType::Enum m_Type = ConnectionType::Execution;
    ezUInt32 m_uiSourcePinIndex = 0;
  };

  AstNode* BuildAST(const ezDocumentObject* pEntryNode);
  void MarkAsCoroutine(AstNode* pEntryAstNode);
  ezResult ReplaceUnsupportedNodes(AstNode* pEntryAstNode);
  ezResult ReplaceLoop(Connection& connection);
  ezResult InsertTypeConversions(AstNode* pEntryAstNode);
  ezResult InlineConstants(AstNode* pEntryAstNode);
  ezResult InlineVariables(AstNode* pEntryAstNode);
  ezResult BuildDataStack(AstNode* pEntryAstNode, ezDynamicArray<AstNode*>& out_Stack);
  ezResult BuildDataExecutions(AstNode* pEntryAstNode);
  ezResult FillDataOutputConnections(AstNode* pEntryAstNode);
  ezResult AssignLocalVariables(AstNode* pEntryAstNode, ezVisualScriptDataDescription& inout_localDataDesc);
  ezResult BuildNodeDescriptions(AstNode* pEntryAstNode, ezDynamicArray<ezVisualScriptNodeDescription>& out_NodeDescriptions);

  struct ConnectionHasher
  {
    static ezUInt32 Hash(const Connection& c);
    static bool Equal(const Connection& a, const Connection& b);
  };

  enum class VisitorResult
  {
    Continue,
    Skip,
    Stop,
    Error,
  };

  using AstNodeVisitorFunc = ezDelegate<VisitorResult(Connection& connection)>;
  ezResult TraverseExecutionConnections(AstNode* pEntryAstNode, AstNodeVisitorFunc func, bool bDeduplicate = true);
  ezResult TraverseDataConnections(AstNode* pEntryAstNode, AstNodeVisitorFunc func, bool bDeduplicate = true, bool bClearReportedConnections = true);
  ezResult TraverseAllConnections(AstNode* pEntryAstNode, AstNodeVisitorFunc func, bool bDeduplicate = true);

  ezResult FinalizeDataOffsets();
  ezResult FinalizeConstantData();

  void DumpAST(AstNode* pEntryAstNode, ezStringView sOutputPath, ezStringView sFunctionName, ezStringView sSuffix);
  void DumpGraph(ezArrayPtr<const ezVisualScriptNodeDescription> nodeDescriptions, ezStringView sOutputPath, ezStringView sFunctionName, ezStringView sSuffix);

  const ezVisualScriptNodeManager* m_pManager = nullptr;

  ezDeque<AstNode> m_AstNodes;
  ezHybridArray<AstNode*, 8> m_EntryAstNodes;
  ezHashTable<const ezRTTI*, DefaultInput> m_DefaultInputs;

  ezHashSet<Connection, ConnectionHasher> m_ReportedConnections;

  ezHashTable<const ezVisualScriptPin*, ezUInt32> m_PinToId;
  ezUInt32 m_uiNextPinId = 0;

  struct DataDesc
  {
    EZ_DECLARE_POD_TYPE();

    DataOffset m_DataOffset;
    ezUInt32 m_uiUsageCounter = 0;
  };

  ezHashTable<ezUInt32, DataDesc> m_PinIdToDataDesc;

  ezHashTable<ezVariant, ezUInt32> m_ConstantDataToIndex;

  CompiledModule m_Module;
};
