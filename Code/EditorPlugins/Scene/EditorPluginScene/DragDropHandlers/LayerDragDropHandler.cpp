#include <EditorPluginScene/EditorPluginScenePCH.h>

#include "EditorFramework/GUI/RawDocumentTreeModel.moc.h"
#include <Core/World/GameObject.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/LayerDragDropHandler.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <QIODevice>
#include <QMimeData>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/DocumentManager.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLayerDragDropHandler, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

const ezRTTI* ezLayerDragDropHandler::GetCommonBaseType(const ezDragDropInfo* pInfo) const
{
  QByteArray encodedData = pInfo->m_pMimeData->data("application/ezEditor.ObjectSelection");
  QDataStream stream(&encodedData, QIODevice::ReadOnly);
  ezHybridArray<ezDocumentObject*, 32> Dragged;
  stream >> Dragged;

  const ezRTTI* pCommonBaseType = nullptr;
  for (const ezDocumentObject* pItem : Dragged)
  {
    pCommonBaseType = pCommonBaseType == nullptr ? pItem->GetType() : ezReflectionUtils::GetCommonBaseType(pCommonBaseType, pItem->GetType());
  }
  return pCommonBaseType;
}


//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLayerOnLayerDragDropHandler, 1, ezRTTIDefaultAllocator<ezLayerOnLayerDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

float ezLayerOnLayerDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (pInfo->m_sTargetContext == "layertree" && pInfo->m_pMimeData->hasFormat("application/ezEditor.ObjectSelection"))
  {
    if (ezScene2Document* pDoc = ezDynamicCast<ezScene2Document*>(ezDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument)))
    {
      const ezDocumentObject* pTarget = pDoc->GetSceneObjectManager()->GetObject(pInfo->m_TargetObject);
      if (pTarget && pInfo->m_pAdapter && GetCommonBaseType(pInfo)->IsDerivedFrom(ezGetStaticRTTI<ezSceneLayerBase>()))
      {
        const ezAbstractProperty* pTargetProp = pInfo->m_pAdapter->GetType()->FindPropertyByName(pInfo->m_pAdapter->GetChildProperty());
        if (pTargetProp && ezGetStaticRTTI<ezSceneLayerBase>()->IsDerivedFrom(pTargetProp->GetSpecificType()))
          return 1.0f;
      }
    }
  }
  return 0;
}

void ezLayerOnLayerDragDropHandler::OnDrop(const ezDragDropInfo* pInfo)
{
  if (ezScene2Document* pDoc = ezDynamicCast<ezScene2Document*>(ezDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument)))
  {
    const ezUuid activeDoc = pDoc->GetActiveLayer();
    EZ_VERIFY(pDoc->SetActiveLayer(pDoc->GetGuid()).Succeeded(), "Failed to set active document.");
    {
      // We need to make a copy of the info as the target document is actually the scene here, not the active document.
      ezDragDropInfo info = *pInfo;
      info.m_TargetDocument = pDoc->GetGuid();
      ezQtDocumentTreeModel::MoveObjects(info);
    }
    EZ_VERIFY(pDoc->SetActiveLayer(activeDoc).Succeeded(), "Failed to set active document.");
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameObjectOnLayerDragDropHandler, 1, ezRTTIDefaultAllocator<ezGameObjectOnLayerDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

float ezGameObjectOnLayerDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (pInfo->m_sTargetContext == "layertree" && pInfo->m_pMimeData->hasFormat("application/ezEditor.ObjectSelection"))
  {
    if (ezScene2Document* pDoc = ezDynamicCast<ezScene2Document*>(ezDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument)))
    {
      const ezDocumentObject* pTarget = pDoc->GetSceneObjectManager()->GetObject(pInfo->m_TargetObject);
      if (pTarget && pTarget->GetType() == ezGetStaticRTTI<ezSceneLayer>() && pInfo->m_iTargetObjectInsertChildIndex == -1 && GetCommonBaseType(pInfo) == ezGetStaticRTTI<ezGameObject>())
      {
        ezObjectAccessorBase* pAccessor = pDoc->GetSceneObjectAccessor();
        ezUuid layerGuid = pAccessor->GetByName<ezUuid>(pTarget, "Layer");
        if (pDoc->IsLayerLoaded(layerGuid))
          return 1.0f;
      }
    }
  }
  return 0;
}

void ezGameObjectOnLayerDragDropHandler::OnDrop(const ezDragDropInfo* pInfo)
{
  if (ezScene2Document* pDoc = ezDynamicCast<ezScene2Document*>(ezDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument)))
  {
    const ezDocumentObject* pTarget = pDoc->GetSceneObjectManager()->GetObject(pInfo->m_TargetObject);

    QByteArray encodedData = pInfo->m_pMimeData->data("application/ezEditor.ObjectSelection");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    ezHybridArray<ezDocumentObject*, 32> Dragged;
    stream >> Dragged;

    // We are dragging game objects on another layer => delete objects and recreate in target layer.
    ezSceneDocument* pSourceDoc = ezDynamicCast<ezSceneDocument*>(Dragged[0]->GetDocumentObjectManager()->GetDocument());
    ezObjectAccessorBase* pAccessor = pDoc->GetSceneObjectAccessor();
    ezUuid layerGuid = pAccessor->GetByName<ezUuid>(pTarget, "Layer");
    ezSceneDocument* pTargetDoc = pDoc->GetLayerDocument(layerGuid);

    if (pSourceDoc != pTargetDoc && pTargetDoc)
    {
      const ezUuid activeDoc = pDoc->GetActiveLayer();
      {
        // activeDoc should already match pSourceDoc, but just to be sure.
        EZ_VERIFY(pDoc->SetActiveLayer(pSourceDoc->GetGuid()).Succeeded(), "Failed to set active document.");

        ezResult res = ezActionManager::ExecuteAction(nullptr, "Selection.Copy", pSourceDoc, ezVariant());
        if (res.Failed())
        {
          ezLog::Error("Failed to copy selection while moving objects between layers.");
          return;
        }
        res = ezActionManager::ExecuteAction(nullptr, "Selection.Delete", pSourceDoc, ezVariant());
        if (res.Failed())
        {
          ezLog::Error("Failed to copy selection while moving objects between layers.");
          return;
        }
      }
      {
        EZ_VERIFY(pDoc->SetActiveLayer(pTargetDoc->GetGuid()).Succeeded(), "Failed to set active document.");
        ezResult res = ezActionManager::ExecuteAction(nullptr, "Selection.PasteAtOriginalLocation", pTargetDoc, ezVariant());
        if (res.Failed())
        {
          ezLog::Error("Failed to paste selection while moving objects between layers.");
          return;
        }
      }
      EZ_VERIFY(pDoc->SetActiveLayer(activeDoc).Succeeded(), "Failed to set active document.");
    }
  }
}
