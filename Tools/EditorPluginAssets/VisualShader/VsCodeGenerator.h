#include <PCH.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>

class ezDocumentNodeManager;

class ezVisualShaderCodeGenerator
{
public:
  ezVisualShaderCodeGenerator();

  ezStatus GenerateVisualShader(const ezDocumentNodeManager* pNodeMaanger, const char* szPlatform, ezStringBuilder& out_sCheckPerms);

  const char* GetFinalShaderCode() const { return m_sFinalShaderCode; }

  void DetermineConfigFileDependencies(const ezDocumentNodeManager* pNodeManager, ezSet<ezString>& out_cfgFiles);

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
    OutputPinState()
    {
      m_bCodeGenerated = false;
    }

    bool m_bCodeGenerated;
    ezString m_sCodeAtPin;
  };


  ezStatus GatherAllNodes(const ezDocumentObject* pRootObj);
  ezUInt16 DeterminePinId(const ezDocumentObject* pOwner, const ezPin* pPin) const;
  ezStatus GenerateNode(const ezDocumentObject* pNode);
  ezStatus GenerateInputPinCode(ezArrayPtr<ezPin* const> pins);
  ezStatus InsertPropertyValues(const ezDocumentObject* pNode, const ezVisualShaderNodeDescriptor* pDesc, ezStringBuilder& sString) const;
  ezStatus GenerateOutputPinCode(const ezDocumentObject* pOwnerNode, const ezPin* pPinSource);

  ezStatus ReplaceInputPinsByCode(const ezDocumentObject* pOwnerNode, const ezVisualShaderNodeDescriptor* pNodeDesc, ezStringBuilder &sInlineCode);
  static void AppendStringIfUnique(ezStringBuilder& inout_String, const char* szAppend);

  const ezDocumentObject* m_pMainNode;
  const ezVisualShaderTypeRegistry* m_pTypeRegistry;
  const ezDocumentNodeManager* m_pNodeManager;
  const ezRTTI* m_pNodeBaseRtti;
  ezMap<const ezDocumentObject*, NodeState> m_Nodes;
  ezMap<const ezPin*, OutputPinState> m_OutputPins;

  ezStringBuilder m_sShaderPixelDefines;
  ezStringBuilder m_sShaderPixelIncludes;
  ezStringBuilder m_sShaderPixelConstants;
  ezStringBuilder m_sShaderPixelSamplers;
  ezStringBuilder m_sShaderPixelBody;
  ezStringBuilder m_sShaderVertex;
  ezStringBuilder m_sShaderMaterialParam;
  ezStringBuilder m_sShaderMaterialCB;
  ezStringBuilder m_sShaderRenderState;
  ezStringBuilder m_sShaderPermutations;
  ezStringBuilder m_sFinalShaderCode;
};

