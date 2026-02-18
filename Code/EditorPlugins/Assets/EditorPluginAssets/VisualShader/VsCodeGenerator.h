#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>

class ezVisualGraphObjectManager;

class ezVisualShaderCodeGenerator
{
public:
  ezVisualShaderCodeGenerator();

  ezStatus GenerateVisualShader(const ezVisualGraphObjectManager* pNodeMaanger, ezStringBuilder& out_sCheckPerms);

  const char* GetFinalShaderCode() const { return m_sFinalShaderCode; }

  void DetermineConfigFileDependencies(const ezVisualGraphObjectManager* pNodeManager, ezSet<ezString>& out_cfgFiles);

private:
  struct NodeState
  {
    NodeState()
    {
      m_uiNodeId = 0;
      m_bCodeGenerated = false;
      m_bInProgress = false;
    }

    ezUInt16 m_uiNodeId;
    bool m_bCodeGenerated;
    bool m_bInProgress;
  };

  struct OutputPinState
  {
    OutputPinState() { m_bCodeGenerated = false; }

    bool m_bCodeGenerated;
    ezString m_sCodeAtPin;
  };


  ezStatus GatherAllNodes(const ezDocumentObject* pRootObj);
  ezUInt16 DeterminePinId(const ezDocumentObject* pOwner, const ezVisualGraphPin& pin) const;
  ezStatus GenerateNode(const ezDocumentObject* pNode);
  ezStatus GenerateInputPinCode(ezArrayPtr<const ezUniquePtr<const ezVisualGraphPin>> pins);
  ezStatus CheckPropertyValues(const ezDocumentObject* pNode, const ezVisualShaderNodeDescriptor* pDesc);
  ezStatus InsertPropertyValues(const ezDocumentObject* pNode, const ezVisualShaderNodeDescriptor* pDesc, ezStringBuilder& sString);
  ezStatus GenerateOutputPinCode(const ezDocumentObject* pOwnerNode, const ezVisualGraphPin& pinSource);

  ezStatus ReplaceInputPinsByCode(const ezDocumentObject* pOwnerNode, const ezVisualShaderNodeDescriptor* pNodeDesc, ezStringBuilder& sInlineCode, ezStringBuilder& sCodeForPlacingDefines);
  void ReplaceMainNodeInputPins(const ezDocumentObject* pMainNode, const ezVisualShaderNodeDescriptor* pNodeDesc, ezStringBuilder& sInlineCode, ezStringBuilder& sCodeForPlacingDefines, ezStringBuilder& out_sHelperFunctions);
  void SetPinDefines(const ezDocumentObject* pOwnerNode, ezStringBuilder& sInlineCode);
  static void AppendStringIfUnique(ezStringBuilder& inout_String, ezStringView sAppend);

  // Generates the transitive hull of nodes connected via input pins to pNode.
  void CollectReachableNodes(const ezDocumentObject* pNode, ezHashSet<const ezDocumentObject*>& out_Nodes) const;

  // Gets the value to use for an unconnected input pin (either from property or default value)
  // Optionally appends defines to pDefinesOut when using a default value
  ezString GetInputPinDefaultValue(const ezDocumentObject* pNode, const ezVisualShaderPinDescriptor& pinDesc, ezStringBuilder* pDefinesOut = nullptr);

  // Collects all nodes that are connected to the given connection. out_Sorted is sorted with the first element being the source pin of pStartConnection.
  ezResult CollectNodesInTopologicalOrder(const ezDocumentObject* pRootNode, ezDynamicArray<const ezDocumentObject*>& out_Sorted) const;

  // Computes the effective output type dimension for all output pins and stores in m_OutputPinDimensions
  void ComputeOutputPinDimensions();

  // Generates a helper function for a main node input pin, returns the function code and the function name
  void GenerateInputHelperFunction(const ezVisualGraphPin* pInputPin, ezUInt32 uiInputIndex, ezStringBuilder& out_sFunctionCode, ezStringBuilder& out_sFunctionCall);

  const ezDocumentObject* m_pMainNode;
  const ezVisualShaderTypeRegistry* m_pTypeRegistry;
  const ezVisualGraphObjectManager* m_pNodeManager;
  const ezRTTI* m_pNodeBaseRtti;
  ezMap<const ezDocumentObject*, NodeState> m_Nodes;
  ezMap<const ezVisualGraphPin*, OutputPinState> m_OutputPins;
  ezMap<const ezVisualGraphPin*, ezUInt8> m_OutputPinDimensions; // Effective output type dimension for each output pin (1-4 for float-float4)
  ezMap<ezString, ezString> m_UsedIdentifiers; // Maps identifier name to node type name
  ezMap<ezString, ezString> m_MaterialParameter;

  ezStringBuilder m_sShaderPixelDefines;
  ezStringBuilder m_sShaderPixelIncludes;
  ezStringBuilder m_sShaderPixelConstants;
  ezStringBuilder m_sShaderPixelSamplers;
  ezStringBuilder m_sShaderPixelBody;
  ezStringBuilder m_sShaderVertexDefines;
  ezStringBuilder m_sShaderVertexIncludes;
  ezStringBuilder m_sShaderVertexBody;
  ezStringBuilder m_sShaderMaterialParam;
  ezStringBuilder m_sShaderMaterialConstants;
  ezStringBuilder m_sShaderMaterialCB;
  ezStringBuilder m_sShaderRenderState;
  ezStringBuilder m_sShaderPermutations;
  ezStringBuilder m_sFinalShaderCode;
};
