#include <PCH.h>
#include <EditorPluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementAsset.h>
#include <EditorPluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementAssetManager.h>
#include <EditorPluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementGraph.h>
#include <EditorPluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementNodes.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/ChunkStream.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProceduralPlacementAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezProceduralPlacementAssetDocument::ezProceduralPlacementAssetDocument(const char* szDocumentPath)
  : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ezProceduralPlacementNodeManager), false, false)
{
}

void ezProceduralPlacementAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  ezAssetDocument::UpdateAssetDocumentInfo(pInfo);

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

ezStatus ezProceduralPlacementAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  ezChunkStreamWriter chunk(stream);
  chunk.BeginStream();

  {
    chunk.BeginChunk("Layers", 1);

    ezDynamicArray<const ezDocumentObject*> outputNodes;
    GetAllOutputNodes(outputNodes);

    chunk << outputNodes.GetCount();

    ezProceduralPlacementLayerOutput output;

    for (auto& outputNode : outputNodes)
    {
      auto& typeAccessor = outputNode->GetTypeAccessor();

      output.m_sName = typeAccessor.GetValue("Name").Get<ezString>();

      output.m_ObjectsToPlace.Clear();
      ezUInt32 uiNumObjects = typeAccessor.GetCount("Objects");
      for (ezUInt32 i = 0; i < uiNumObjects; ++i)
      {
        output.m_ObjectsToPlace.PushBack(typeAccessor.GetValue("Objects", i).Get<ezString>());
      }

      output.m_fFootprint = typeAccessor.GetValue("Footprint").Get<float>();

      output.m_vMinOffset = typeAccessor.GetValue("MinOffset").Get<ezVec3>();
      output. m_vMaxOffset = typeAccessor.GetValue("MaxOffset").Get<ezVec3>();

      output.m_fAlignToNormal = typeAccessor.GetValue("AlignToNormal").Get<float>();

      output. m_vMinScale = typeAccessor.GetValue("MinScale").Get<ezVec3>();
      output. m_vMaxScale = typeAccessor.GetValue("MaxScale").Get<ezVec3>();

      output.m_fCullDistance = typeAccessor.GetValue("CullDistance").Get<float>();


      output.Save(chunk);
    }

    chunk.EndChunk();
  }



  chunk.EndStream();

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
    auto pType = pObject->GetTypeAccessor().GetType();
    if (!pType->IsDerivedFrom(pLayerOutputRtti))
      continue;

    allNodes.PushBack(pObject);
  }
}



