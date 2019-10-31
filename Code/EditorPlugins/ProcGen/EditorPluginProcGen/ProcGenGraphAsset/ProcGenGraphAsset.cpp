#include <EditorPluginProcGenPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAssetManager.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodeManager.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <ProcGenPlugin/VM/ExpressionByteCode.h>
#include <ProcGenPlugin/VM/ExpressionCompiler.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

namespace
{
  void DumpAST(const ezExpressionAST& ast, ezStringView sAssetName, ezStringView sOutputName)
  {
    ezDGMLGraph dgmlGraph;
    ast.PrintGraph(dgmlGraph);

    ezStringBuilder sFileName;
    sFileName.Format(":appdata/{0}_{1}_AST.dgml", sAssetName, sOutputName);

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

  static const char* s_szSphereAssetId = "{ a3ce5d3d-be5e-4bda-8820-b1ce3b3d33fd }"; // Base/Prefabs/Sphere.ezPrefab
  static const char* s_szBWGradientAssetId =
    "{ 3834b7d0-5a3f-140d-31d8-3a2bf48b09bd }"; // Base/Textures/BlackWhiteGradient.ezColorGradientAsset

} // namespace

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGenGraphAssetDocument, 4, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezProcGenGraphAssetDocument::ezProcGenGraphAssetDocument(const char* szDocumentPath)
  : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ezProcGenNodeManager), ezAssetDocEngineConnection::Simple)
{
}

void ezProcGenGraphAssetDocument::SetDebugPin(const ezPin* pDebugPin)
{
  m_pDebugPin = pDebugPin;

  if (m_pDebugPin != nullptr)
  {
    CreateDebugNode();
  }

  ezDocumentObjectPropertyEvent e;
  e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertySet;
  e.m_sProperty = "DebugPin";

  GetObjectManager()->m_PropertyEvents.Broadcast(e);
}

ezStatus ezProcGenGraphAssetDocument::WriteAsset(ezStreamWriter& stream, const ezPlatformProfile* pAssetProfile) const
{
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());

  ezAbstractObjectGraph graph;
  ezDocumentObjectConverterWriter objectWriter(&graph, pManager);

  ezRttiConverterContext rttiConverterContext;
  ezRttiConverterReader rttiConverter(&graph, &rttiConverterContext);

  ezHashTable<const ezDocumentObject*, CachedNode> nodeCache;

  ezDynamicArray<const ezDocumentObject*> placementNodes;
  ezDynamicArray<const ezDocumentObject*> vertexColorNodes;
  GetAllOutputNodes(placementNodes, vertexColorNodes);

  const bool bDebug = m_pDebugPin != nullptr;

  ezChunkStreamWriter chunk(stream);
  chunk.BeginStream(1);

  ezExpressionCompiler compiler;
  ezProcGenNodeBase::GenerateASTContext context;

  auto WriteByteCode = [&](const ezDocumentObject* pOutputNode) {
    context.m_VolumeTagSetIndices.Clear();

    ezExpressionAST ast;
    GenerateExpressionAST(pOutputNode, objectWriter, rttiConverter, nodeCache, ast, context);

    if (false)
    {
      ezStringBuilder sDocumentPath = GetDocumentPath();
      ezStringView sAssetName = sDocumentPath.GetFileNameAndExtension();
      ezStringView sOutputName = pOutputNode->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>();

      DumpAST(ast, sAssetName, sOutputName);
    }

    ezExpressionByteCode byteCode;
    if (compiler.Compile(ast, byteCode).Failed())
    {
      return ezStatus("Compilation failed");
    }

    byteCode.Save(chunk);

    return ezStatus(EZ_SUCCESS);
  };

  {
    chunk.BeginChunk("PlacementOutputs", 4);

    if (!bDebug)
    {
      chunk << placementNodes.GetCount();

      for (auto pPlacementNode : placementNodes)
      {
        EZ_SUCCEED_OR_RETURN(WriteByteCode(pPlacementNode));

        CachedNode cachedNode;
        EZ_VERIFY(nodeCache.TryGetValue(pPlacementNode, cachedNode), "Implementation error");
        auto pPlacementOutput = ezStaticCast<ezProcGenPlacementOutput*>(cachedNode.m_pPPNode);

        pPlacementOutput->m_VolumeTagSetIndices = context.m_VolumeTagSetIndices;
        pPlacementOutput->Save(chunk);
      }
    }
    else
    {
      ezUInt32 uiNumNodes = 1;
      chunk << uiNumNodes;

      context.m_VolumeTagSetIndices.Clear();

      ezExpressionAST ast;
      GenerateDebugExpressionAST(objectWriter, rttiConverter, nodeCache, ast, context);

      ezExpressionByteCode byteCode;
      if (compiler.Compile(ast, byteCode).Failed())
      {
        return ezStatus("Debug Compilation failed");
      }

      byteCode.Save(chunk);

      m_pDebugNode->m_VolumeTagSetIndices = context.m_VolumeTagSetIndices;
      m_pDebugNode->Save(chunk);
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("VertexColorOutputs", 2);

    chunk << vertexColorNodes.GetCount();

    for (auto pVertexColorNode : vertexColorNodes)
    {
      EZ_SUCCEED_OR_RETURN(WriteByteCode(pVertexColorNode));

      CachedNode cachedNode;
      EZ_VERIFY(nodeCache.TryGetValue(pVertexColorNode, cachedNode), "Implementation error");
      auto pVertexColorOutput = ezStaticCast<ezProcGenVertexColorOutput*>(cachedNode.m_pPPNode);

      pVertexColorOutput->m_VolumeTagSetIndices = context.m_VolumeTagSetIndices;
      pVertexColorOutput->Save(chunk);
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("SharedData", 1);

    context.m_SharedData.Save(chunk);

    chunk.EndChunk();
  }

  chunk.EndStream();

  for (auto it = nodeCache.GetIterator(); it.IsValid(); ++it)
  {
    rttiConverterContext.DeleteObject(it.Key()->GetGuid());
  }

  return ezStatus(EZ_SUCCESS);
}

void ezProcGenGraphAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (m_pDebugPin == nullptr)
  {
    ezDynamicArray<const ezDocumentObject*> placementNodes;
    ezDynamicArray<const ezDocumentObject*> vertexColorNodes;
    GetAllOutputNodes(placementNodes, vertexColorNodes);

    for (auto pPlacementNode : placementNodes)
    {
      auto& typeAccessor = pPlacementNode->GetTypeAccessor();

      ezUInt32 uiNumObjects = typeAccessor.GetCount("Objects");
      for (ezUInt32 i = 0; i < uiNumObjects; ++i)
      {
        ezVariant prefab = typeAccessor.GetValue("Objects", i);
        if (prefab.IsA<ezString>())
        {
          pInfo->m_RuntimeDependencies.Insert(prefab.Get<ezString>());
        }
      }

      ezVariant colorGradient = typeAccessor.GetValue("ColorGradient");
      if (colorGradient.IsA<ezString>())
      {
        pInfo->m_RuntimeDependencies.Insert(colorGradient.Get<ezString>());
      }
    }
  }
  else
  {
    pInfo->m_RuntimeDependencies.Insert(s_szSphereAssetId);
    pInfo->m_RuntimeDependencies.Insert(s_szBWGradientAssetId);
  }
}

ezStatus ezProcGenGraphAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag,
  const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  EZ_ASSERT_DEV(ezStringUtils::IsNullOrEmpty(szOutputTag), "Additional output '{0}' not implemented!", szOutputTag);

  return WriteAsset(stream, pAssetProfile);
}

void ezProcGenGraphAssetDocument::GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/ezEditor.ProcGenGraph");
}

bool ezProcGenGraphAssetDocument::CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const
{
  out_MimeType = "application/ezEditor.ProcGenGraph";

  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return false;

  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());

  ezDocumentObjectConverterWriter writer(&out_objectGraph, pManager);

  for (const ezDocumentObject* pNode : selection)
  {
    // objects are required to be named root but this is not enforced or obvious by the interface.
    writer.AddObjectToGraph(pNode, "root");
  }

  pManager->AttachMetaDataBeforeSaving(out_objectGraph);

  return true;
}

bool ezProcGenGraphAssetDocument::Paste(
  const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
{
  bool bAddedAll = true;

  ezDeque<const ezDocumentObject*> AddedNodes;

  for (const PasteInfo& pi : info)
  {
    // only add nodes that are allowed to be added
    if (GetObjectManager()->CanAdd(pi.m_pObject->GetTypeAccessor().GetType(), nullptr, "Children", pi.m_Index).m_Result.Succeeded())
    {
      AddedNodes.PushBack(pi.m_pObject);
      GetObjectManager()->AddObject(pi.m_pObject, nullptr, "Children", pi.m_Index);
    }
    else
    {
      bAddedAll = false;
    }
  }

  m_DocumentObjectMetaData.RestoreMetaDataFromAbstractGraph(objectGraph);

  RestoreMetaDataAfterLoading(objectGraph, true);

  if (!AddedNodes.IsEmpty() && bAllowPickedPosition)
  {
    ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());

    ezVec2 vAvgPos(0);
    for (const ezDocumentObject* pNode : AddedNodes)
    {
      vAvgPos += pManager->GetNodePos(pNode);
    }

    vAvgPos /= AddedNodes.GetCount();

    const ezVec2 vMoveNode = -vAvgPos + ezQtNodeScene::GetLastMouseInteractionPos();

    for (const ezDocumentObject* pNode : AddedNodes)
    {
      ezMoveNodeCommand move;
      move.m_Object = pNode->GetGuid();
      move.m_NewPos = pManager->GetNodePos(pNode) + vMoveNode;
      GetCommandHistory()->AddCommand(move);
    }

    if (!bAddedAll)
    {
      ezLog::Info("[EditorStatus]Not all nodes were allowed to be added to the document");
    }
  }

  GetSelectionManager()->SetSelection(AddedNodes);
  return true;
}

void ezProcGenGraphAssetDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void ezProcGenGraphAssetDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable)
{
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}

void ezProcGenGraphAssetDocument::GetAllOutputNodes(
  ezDynamicArray<const ezDocumentObject*>& placementNodes, ezDynamicArray<const ezDocumentObject*>& vertexColorNodes) const
{
  const ezRTTI* pPlacementOutputRtti = ezGetStaticRTTI<ezProcGenPlacementOutput>();
  const ezRTTI* pVertexColorOutputRtti = ezGetStaticRTTI<ezProcGenVertexColorOutput>();

  placementNodes.Clear();
  vertexColorNodes.Clear();

  const auto& children = GetObjectManager()->GetRootObject()->GetChildren();
  for (const ezDocumentObject* pObject : children)
  {
    if (pObject->GetTypeAccessor().GetValue("Active").ConvertTo<bool>())
    {
      const ezRTTI* pRtti = pObject->GetTypeAccessor().GetType();
      if (pRtti->IsDerivedFrom(pPlacementOutputRtti))
      {
        placementNodes.PushBack(pObject);
      }
      else if (pRtti->IsDerivedFrom(pVertexColorOutputRtti))
      {
        vertexColorNodes.PushBack(pObject);
      }
    }
  }
}

ezExpressionAST::Node* ezProcGenGraphAssetDocument::GenerateExpressionAST(const ezDocumentObject* outputNode,
  ezDocumentObjectConverterWriter& objectWriter, ezRttiConverterReader& rttiConverter,
  ezHashTable<const ezDocumentObject*, CachedNode>& nodeCache, ezExpressionAST& out_Ast, ezProcGenNodeBase::GenerateASTContext& context) const
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
    inputAstNodes[i] = GenerateExpressionAST(pPinSource->GetParent(), objectWriter, rttiConverter, nodeCache, out_Ast, context);
  }

  CachedNode cachedNode;
  if (!nodeCache.TryGetValue(outputNode, cachedNode))
  {
    ezAbstractObjectNode* pAbstractNode = objectWriter.AddObjectToGraph(outputNode);
    cachedNode.m_pPPNode = static_cast<ezProcGenNodeBase*>(rttiConverter.CreateObjectFromNode(pAbstractNode));

    nodeCache.Insert(outputNode, cachedNode);
  }

  return cachedNode.m_pPPNode->GenerateExpressionASTNode(inputAstNodes, out_Ast, context);
}

ezExpressionAST::Node* ezProcGenGraphAssetDocument::GenerateDebugExpressionAST(ezDocumentObjectConverterWriter& objectWriter,
  ezRttiConverterReader& rttiConverter, ezHashTable<const ezDocumentObject*, CachedNode>& nodeCache, ezExpressionAST& out_Ast,
  ezProcGenNodeBase::GenerateASTContext& context) const
{
  EZ_ASSERT_DEV(m_pDebugPin != nullptr, "");

  const ezPin* pPinSource = m_pDebugPin;
  if (pPinSource->GetType() == ezPin::Type::Input)
  {
    auto connections = pPinSource->GetConnections();
    EZ_ASSERT_DEBUG(connections.GetCount() <= 1, "Input pin has {0} connections", connections.GetCount());

    if (connections.IsEmpty())
      return nullptr;

    pPinSource = connections[0]->GetSourcePin();
    EZ_ASSERT_DEBUG(pPinSource != nullptr, "Invalid connection");
  }

  ezHybridArray<ezExpressionAST::Node*, 8> inputAstNodes;
  inputAstNodes.SetCount(4); // placement output node has 4 inputs

  // Recursively generate all dependent code and pretend it is connected to the color index input of the debug placement output node.
  inputAstNodes[2] = GenerateExpressionAST(pPinSource->GetParent(), objectWriter, rttiConverter, nodeCache, out_Ast, context);

  return m_pDebugNode->GenerateExpressionASTNode(inputAstNodes, out_Ast, context);
}

void ezProcGenGraphAssetDocument::DumpSelectedOutput(bool bAst, bool bDisassembly) const
{
  const ezDocumentObject* pSelectedNode = nullptr;

  auto selection = GetSelectionManager()->GetSelection();
  if (!selection.IsEmpty())
  {
    pSelectedNode = selection[0];
    if (!pSelectedNode->GetType()->IsDerivedFrom(ezGetStaticRTTI<ezProcGenOutput>()))
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

  ezRttiConverterContext rttiConverterContext;
  ezRttiConverterReader rttiConverter(&graph, &rttiConverterContext);

  ezHashTable<const ezDocumentObject*, CachedNode> nodeCache;

  ezExpressionAST ast;
  ezProcGenNodeBase::GenerateASTContext context;
  GenerateExpressionAST(pSelectedNode, objectWriter, rttiConverter, nodeCache, ast, context);

  ezStringBuilder sDocumentPath = GetDocumentPath();
  ezStringView sAssetName = sDocumentPath.GetFileNameAndExtension();
  ezStringView sOutputName = pSelectedNode->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>();

  if (bAst)
  {
    DumpAST(ast, sAssetName, sOutputName);
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
      sFileName.Format(":appdata/{0}_{1}_ByteCode.txt", sAssetName, sOutputName);

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
    rttiConverterContext.DeleteObject(it.Key()->GetGuid());
  }
}

void ezProcGenGraphAssetDocument::CreateDebugNode()
{
  if (m_pDebugNode != nullptr)
    return;

  m_pDebugNode = EZ_DEFAULT_NEW(ezProcGenPlacementOutput);
  m_pDebugNode->m_sName = "Debug";
  m_pDebugNode->m_ObjectsToPlace.PushBack(s_szSphereAssetId);
  m_pDebugNode->m_sColorGradient = s_szBWGradientAssetId;
}
