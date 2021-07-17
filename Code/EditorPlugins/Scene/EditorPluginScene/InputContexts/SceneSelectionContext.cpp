#include <EditorPluginScenePCH.h>

#include <EditorPluginScene/InputContexts/SceneSelectionContext.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>

 ezSceneSelectionContext::ezSceneSelectionContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView, const ezCamera* pCamera)
  : ezSelectionContext(pOwnerWindow, pOwnerView, pCamera)
{
}

void ezSceneSelectionContext::OpenDocumentForPickedObject(const ezObjectPickingResult& res) const
{
  ezSelectionContext::OpenDocumentForPickedObject(res);
}

void ezSceneSelectionContext::SelectPickedObject(const ezObjectPickingResult& res, bool bToggle, bool bDirect) const
{
  if (res.m_PickedObject.IsValid())
  {
    ezUuid layerGuid = FindLayerByObject(res.m_PickedObject);
    if (layerGuid.IsValid())
    {
      ezScene2Document* pSceneDocument = ezDynamicCast<ezScene2Document*>(GetOwnerWindow()->GetDocument());
      if (pSceneDocument->IsLayerLoaded(layerGuid))
      {
        pSceneDocument->SetActiveLayer(layerGuid);
      }
    }
  }
  ezSelectionContext::SelectPickedObject(res, bToggle, bDirect);
}

ezUuid ezSceneSelectionContext::FindLayerByObject(ezUuid objectGuid) const
{
  ezHybridArray<ezSceneDocument*, 8> loadedLayers;
  const ezScene2Document* pSceneDocument = ezDynamicCast<const ezScene2Document*>(GetOwnerWindow()->GetDocument());
  pSceneDocument->GetLoadedLayers(loadedLayers);
  for (ezSceneDocument* pLayer : loadedLayers)
  {
    if (pLayer == pSceneDocument)
    {
      if (pSceneDocument->GetSceneObjectManager()->GetObject(objectGuid))
      {
        return pLayer->GetGuid();
      }
    }
    else if (pLayer->GetObjectManager()->GetObject(objectGuid) != nullptr)
    {
      return pLayer->GetGuid();
    }
  }
  return ezUuid();
}
