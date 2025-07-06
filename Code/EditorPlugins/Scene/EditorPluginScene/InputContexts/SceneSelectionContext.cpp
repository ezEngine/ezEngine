#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorPluginScene/InputContexts/SceneSelectionContext.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
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
  ezScene2Document* pSceneDocument = nullptr;

  // If bToggle (ctrl-key) is held, we don't want to switch layers.
  // Same if we have a custom pick override set which usually means that the selection is hijacked to make an object modification on the current layer.
  if (res.m_PickedObject.IsValid() && !bToggle)
  {
    const ezDocumentObject* pObject = nullptr;
    ezUuid layerGuid = FindLayerByObject(res.m_PickedObject, pObject);
    if (layerGuid.IsValid())
    {
      pSceneDocument = ezDynamicCast<ezScene2Document*>(GetOwnerWindow()->GetDocument());
      if (pSceneDocument->IsLayerLoaded(layerGuid))
      {
        if (m_PickObjectOverride.IsValid())
        {
          m_PickObjectOverride(pObject);
          return;
        }

        if (pSceneDocument->GetActiveLayer() != layerGuid)
        {
          if (pSceneDocument->GetSwitchLayerToSelection())
          {
            pSceneDocument->PreventDoubleSelectionChange(true);
            pSceneDocument->SetActiveLayer(layerGuid).LogFailure();
          }
          else
          {
            pSceneDocument->ShowDocumentStatus(ezFmt("The clicked object is in layer '{}'. Switch layer or enable 'Auto Switch Layer to Selection'", pSceneDocument->GetLayerDocument(layerGuid)->GetDocumentPath().GetFileName()));
          }
        }
      }
    }
  }

  ezSelectionContext::SelectPickedObject(res, bToggle, bDirect);

  if (pSceneDocument)
    pSceneDocument->PreventDoubleSelectionChange(false);
}

ezUuid ezSceneSelectionContext::FindLayerByObject(ezUuid objectGuid, const ezDocumentObject*& out_pObject) const
{
  ezHybridArray<ezSceneDocument*, 8> loadedLayers;
  const ezScene2Document* pSceneDocument = ezDynamicCast<const ezScene2Document*>(GetOwnerWindow()->GetDocument());
  pSceneDocument->GetLoadedLayers(loadedLayers);
  for (ezSceneDocument* pLayer : loadedLayers)
  {
    if (pLayer == pSceneDocument)
    {
      if ((out_pObject = pSceneDocument->GetSceneObjectManager()->GetObject(objectGuid)))
      {
        return pLayer->GetGuid();
      }
    }
    else if ((out_pObject = pLayer->GetObjectManager()->GetObject(objectGuid)))
    {
      return pLayer->GetGuid();
    }
  }
  out_pObject = nullptr;
  return ezUuid();
}
