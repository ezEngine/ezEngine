#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/MeshAsset/MeshEditorContext.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshEditorInputContext, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezMeshEditorInputContext::ezMeshEditorInputContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  SetOwner(pOwnerWindow, pOwnerView);
}

ezEditorInput ezMeshEditorInputContext::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::MiddleButton && (e->modifiers() & Qt::KeyboardModifier::ControlModifier))
  {
    const ezObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

    auto* pMeshDoc = static_cast<ezMeshAssetDocument*>(GetOwnerWindow()->GetDocument());
    auto& allSlots = pMeshDoc->GetProperties()->m_Slots;

    if (res.m_uiPartIndex < (ezUInt32)allSlots.GetCount())
    {
      if (!allSlots[res.m_uiPartIndex].m_sResource.IsEmpty())
      {
        for (auto* pDocMan : ezDocumentManager::GetAllDocumentManagers())
        {
          if (auto* pAssetMan = ezDynamicCast<ezAssetDocumentManager*>(pDocMan))
          {
            if (pAssetMan->TryOpenAssetDocument(allSlots[res.m_uiPartIndex].m_sResource).Succeeded())
              return ezEditorInput::WasExclusivelyHandled;
          }
        }
        GetOwnerWindow()->ShowTemporaryStatusBarMsg("Could not open material document.");
      }
      else
      {
        GetOwnerWindow()->ShowTemporaryStatusBarMsg("No material assigned to this mesh surface.");
      }
    }
    return ezEditorInput::WasExclusivelyHandled;
  }
  return ezEditorInput::MayBeHandledByOthers;
}
