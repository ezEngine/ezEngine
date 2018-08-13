#include <PCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementAsset.h>
#include <EditorPluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementAssetManager.h>
#include <EditorPluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementGraph.h>
#include <EditorPluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementNodes.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Strings/PathUtils.h>
#include <ProceduralPlacementPlugin/VM/ExpressionByteCode.h>
#include <ProceduralPlacementPlugin/VM/ExpressionCompiler.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Utilities/DGML/DGMLWriter.h>

namespace
{
  void DumpAST(const ezExpressionAST& ast, ezStringView sAssetName, ezStringView sLayerName)
  {
    ezDGMLGraph dgmlGraph;
    ast.PrintGraph(dgmlGraph);

    ezStringBuilder sFileName;
    sFileName.Format(":appdata/{0}_{1}_AST.dgml", sAssetName, sLayerName);

    ezDGMLGraphWriter dgmlGraphWriter;
    EZ_IGNORE_UNUSED(dgmlGraphWriter);
    if (dgmlGraphWriter.WriteGraphToFile(sFileName, dgmlGraph).Succeeded())
    {
      ezLog::Info("AST was dumped to: {0}", sFileName);
    }
    else
    {
      ezLog::Error("Failed to dump AST to: {0}", sFileName);
    }
  }
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProceduralPlacementAssetDocument, 2, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezProceduralPlacementAssetDocument::ezProceduralPlacementAssetDocument(const char* szDocumentPath)
    : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ezProceduralPlacementNodeManager), false, false)
{
}

void ezProceduralPlacementAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  ezDynamicArray<const ezDocumentObject*> outputNodes;
  GetAllOutputNodes(outputNodes);

  for (auto pOutputNode : outputNodes)
  {
    auto& typeAccessor = pOutputNode->GetTypeAccessor();

    ezUInt32 uiNumObjects = typeAccessor.GetCount("Objects");
    for (ezUInt32 i = 0; i < uiNumObjects; ++i)
    {
      ezVariant prefab = typeAccessor.GetValue("Objects", i);
      if (prefab.IsA<ezString>())
      {
        pInfo->m_RuntimeDependencies.Insert(prefab.Get<ezString>());
      }
    }
  }
}

ezStatus ezProceduralPlacementAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform,
                                                                    const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());

  ezAbstractObjectGraph graph;
  ezDocumentObjectConverterWriter objectWriter(&graph, pManager);

  ezRttiConverterContext context;
  ezRttiConverterReader rttiConverter(&graph, &context);

  ezHashTable<const ezDocumentObject*, CachedNode> nodeCache;

  ezDynamicArray<const ezDocumentObject*> outputNodes;
  GetAllOutputNodes(outputNodes);
  ezUInt32 uiNumOutputNodes = outputNodes.GetCount();

  ezChunkStreamWriter chunk(stream);
  chunk.BeginStream(1);

  {
    chunk.BeginChunk("ByteCode", 1);

    chunk << uiNumOutputNodes;

    ezExpressionAST ast;
    ezExpressionCompiler compiler;

    for (ezUInt32 uiOutputNodeIndex = 0; uiOutputNodeIndex < uiNumOutputNodes; ++uiOutputNodeIndex)
    {
      auto pOutputNode = outputNodes[uiOutputNodeIndex];

      GenerateExpressionAST(pOutputNode, objectWriter, rttiConverter, nodeCache, ast);

      if (false)
      {
        ezStringBuilder sDocumentPath = GetDocumentPath();
        ezStringView sAssetName = sDocumentPath.GetFileNameAndExtension();
        ezStringView sLayerName = pOutputNode->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>();

        DumpAST(ast, sAssetName, sLayerName);
      }

      ezExpressionByteCode byteCode;
      if (compiler.Compile(ast, byteCode).Failed())
      {
        return ezStatus("Compilation failed");
      }

      byteCode.Save(chunk);
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("Layers", 2);

    chunk << uiNumOutputNodes;

    for (ezUInt32 uiOutputNodeIndex = 0; uiOutputNodeIndex < uiNumOutputNodes; ++uiOutputNodeIndex)
    {
      CachedNode cachedNode;
      EZ_VERIFY(nodeCache.TryGetValue(outputNodes[uiOutputNodeIndex], cachedNode), "Implementation error");
      auto pOutputNode = static_cast<ezProceduralPlacementLayerOutput*>(cachedNode.m_pPPNode);
      pOutputNode->m_uiByteCodeIndex = uiOutputNodeIndex;

      pOutputNode->Save(chunk);
    }

    chunk.EndChunk();
  }

  chunk.EndStream();

  for (auto it = nodeCache.GetIterator(); it.IsValid(); ++it)
  {
    context.DeleteObject(it.Key()->GetGuid());
  }

  return ezStatus(EZ_SUCCESS);
}

void ezProceduralPlacementAssetDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void ezProceduralPlacementAssetDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable)
{
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}

void ezProceduralPlacementAssetDocument::GetAllOutputNodes(ezDynamicArray<const ezDocumentObject*>& allNodes) const
{
  const ezRTTI* pLayerOutputRtti = ezProceduralPlacementNodeRegistry::GetSingleton()->GetLayerOutputType();

  allNodes.Clear();
  allNodes.Reserve(64);

  const auto& children = GetObjectManager()->GetRootObject()->GetChildren();
  for (const ezDocumentObject* pObject : children)
  {
    const ezRTTI* pRtti = pObject->GetTypeAccessor().GetType();
    if (!pRtti->IsDerivedFrom(pLayerOutputRtti))
      continue;

    if (pObject->GetTypeAccessor().GetValue("Active").ConvertTo<bool>())
    {
      allNodes.PushBack(pObject);
    }
  }
}

ezExpressionAST::Node* ezProceduralPlacementAssetDocument::GenerateExpressionAST(
    const ezDocumentObject* outputNode, ezDocumentObjectConverterWriter& objectWriter, ezRttiConverterReader& rttiConverter,
    ezHashTable<const ezDocumentObject*, CachedNode>& nodeCache, ezExpressionAST& out_Ast) const
{
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());

  const ezArrayPtr<ezPin* const> inputPins = pManager->GetInputPins(outputNode);

  ezHybridArray<ezExpressionAST::Node*, 8> inputAstNodes;
  inputAstNodes.SetCount(inputPins.GetCount());

  for (ezUInt32 i = 0; i < inputPins.GetCount(); ++i)
  {
    auto connections = inputPins[i]->GetConnections();
    EZ_ASSERT_DEBUG(connections.GetCount() <= 1, "Input pin has {0} connections", connections.GetCount());

    if (connections.IsEmpty())
      continue;

    const ezPin* pPinSource = connections[0]->GetSourcePin();
    EZ_ASSERT_DEBUG(pPinSource != nullptr, "Invalid connection");

    // recursively generate all dependent code
    inputAstNodes[i] = GenerateExpressionAST(pPinSource->GetParent(), objectWriter, rttiConverter, nodeCache, out_Ast);
  }

  CachedNode cachedNode;
  if (!nodeCache.TryGetValue(outputNode, cachedNode))
  {
    ezAbstractObjectNode* pAbstractNode = objectWriter.AddObjectToGraph(outputNode);
    cachedNode.m_pPPNode = static_cast<ezProceduralPlacementNodeBase*>(rttiConverter.CreateObjectFromNode(pAbstractNode));

    cachedNode.m_pASTNode = cachedNode.m_pPPNode->GenerateExpressionASTNode(inputAstNodes, out_Ast);
    nodeCache.Insert(outputNode, cachedNode);
  }

  return cachedNode.m_pASTNode;
}


void ezProceduralPlacementAssetDocument::DumpSelectedOutput(bool bAst, bool bDisassembly) const
{
  const ezDocumentObject* pSelectedNode = nullptr;

  auto selection = GetSelectionManager()->GetSelection();
  if (!selection.IsEmpty())
  {
    pSelectedNode = selection[0];
    if (!pSelectedNode->GetType()->IsDerivedFrom(ezProceduralPlacementNodeRegistry::GetSingleton()->GetLayerOutputType()))
    {
      pSelectedNode = nullptr;
    }
  }

  if (pSelectedNode == nullptr)
  {
    ezLog::Error("No valid output node selected.");
    return;
  }

  ezAbstractObjectGraph graph;
  ezDocumentObjectConverterWriter objectWriter(&graph, GetObjectManager());

  ezRttiConverterContext context;
  ezRttiConverterReader rttiConverter(&graph, &context);

  ezHashTable<const ezDocumentObject*, CachedNode> nodeCache;

  ezExpressionAST ast;
  GenerateExpressionAST(pSelectedNode, objectWriter, rttiConverter, nodeCache, ast);

  ezStringBuilder sDocumentPath = GetDocumentPath();
  ezStringView sAssetName = sDocumentPath.GetFileNameAndExtension();
  ezStringView sLayerName = pSelectedNode->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>();

  if (bAst)
  {
    DumpAST(ast, sAssetName, sLayerName);
  }

  if (bDisassembly)
  {
    ezExpressionByteCode byteCode;

    ezExpressionCompiler compiler;
    if (compiler.Compile(ast, byteCode).Succeeded())
    {
      ezStringBuilder sDisassembly;
      byteCode.Disassemble(sDisassembly);

      ezStringBuilder sFileName;
      sFileName.Format(":appdata/{0}_ByteCode.txt", sAssetName);

      ezFileWriter fileWriter;
      if (fileWriter.Open(sFileName).Succeeded())
      {
        fileWriter.WriteBytes(sDisassembly.GetData(), sDisassembly.GetElementCount());

        ezLog::Info("Disassembly was dumped to: {0}", sFileName);
      }
      else
      {
        ezLog::Error("Failed to dump Disassembly to: {0}", sFileName);
      }
    }
    else
    {
      ezLog::Error("Compiling expression failed");
    }
  }

  for (auto it = nodeCache.GetIterator(); it.IsValid(); ++it)
  {
    context.DeleteObject(it.Key()->GetGuid());
  }
}
