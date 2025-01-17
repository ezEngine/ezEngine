#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginScene/Actions/LayerActions.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <QInputDialog>


// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLayerAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActionDescriptorHandle ezLayerActions::s_hLayerCategory;
ezActionDescriptorHandle ezLayerActions::s_hCreateLayer;
ezActionDescriptorHandle ezLayerActions::s_hDeleteLayer;
ezActionDescriptorHandle ezLayerActions::s_hSaveLayer;
ezActionDescriptorHandle ezLayerActions::s_hSaveActiveLayer;
ezActionDescriptorHandle ezLayerActions::s_hLayerLoaded;
ezActionDescriptorHandle ezLayerActions::s_hLayerVisible;

void ezLayerActions::RegisterActions()
{
  s_hLayerCategory = EZ_REGISTER_CATEGORY("LayerCategory");

  s_hCreateLayer = EZ_REGISTER_ACTION_1("Layer.CreateLayer", ezActionScope::Document, "Scene - Layer", "",
    ezLayerAction, ezLayerAction::ActionType::CreateLayer);
  s_hDeleteLayer = EZ_REGISTER_ACTION_1("Layer.DeleteLayer", ezActionScope::Document, "Scene - Layer", "",
    ezLayerAction, ezLayerAction::ActionType::DeleteLayer);
  s_hSaveLayer = EZ_REGISTER_ACTION_1("Layer.SaveLayer", ezActionScope::Document, "Scene - Layer", "",
    ezLayerAction, ezLayerAction::ActionType::SaveLayer);
  s_hSaveActiveLayer = EZ_REGISTER_ACTION_1("Layer.SaveActiveLayer", ezActionScope::Document, "Scene - Layer", "Ctrl+S",
    ezLayerAction, ezLayerAction::ActionType::SaveActiveLayer);
  s_hLayerLoaded = EZ_REGISTER_ACTION_1("Layer.LayerLoaded", ezActionScope::Document, "Scene - Layer", "",
    ezLayerAction, ezLayerAction::ActionType::LayerLoaded);
  s_hLayerVisible = EZ_REGISTER_ACTION_1("Layer.LayerVisible", ezActionScope::Document, "Scene - Layer", "",
    ezLayerAction, ezLayerAction::ActionType::LayerVisible);
}

void ezLayerActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hLayerCategory);
  ezActionManager::UnregisterAction(s_hCreateLayer);
  ezActionManager::UnregisterAction(s_hDeleteLayer);
  ezActionManager::UnregisterAction(s_hSaveLayer);
  ezActionManager::UnregisterAction(s_hSaveActiveLayer);
  ezActionManager::UnregisterAction(s_hLayerLoaded);
  ezActionManager::UnregisterAction(s_hLayerVisible);
}

void ezLayerActions::MapContextMenuActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);


  pMap->MapAction(s_hLayerCategory, "", 0.0f);

  const ezStringView sSubPath = "LayerCategory";
  pMap->MapAction(s_hCreateLayer, sSubPath, 1.0f);
  pMap->MapAction(s_hDeleteLayer, sSubPath, 2.0f);
  pMap->MapAction(s_hSaveLayer, sSubPath, 3.0f);
  pMap->MapAction(s_hLayerLoaded, sSubPath, 4.0f);
  pMap->MapAction(s_hLayerVisible, sSubPath, 5.0f);
}

ezLayerAction::ezLayerAction(const ezActionContext& context, const char* szName, ezLayerAction::ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;
  // TODO const cast
  m_pSceneDocument = const_cast<ezScene2Document*>(static_cast<const ezScene2Document*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::CreateLayer:
      SetIconPath(":/GuiFoundation/Icons/Add.svg");
      break;
    case ActionType::DeleteLayer:
      SetIconPath(":/GuiFoundation/Icons/Delete.svg");
      break;
    case ActionType::SaveLayer:
    case ActionType::SaveActiveLayer:
      SetIconPath(":/GuiFoundation/Icons/Save.svg");
      break;
    case ActionType::LayerLoaded:
      SetCheckable(true);
      break;
    case ActionType::LayerVisible:
      SetCheckable(true);
      break;
  }

  UpdateEnableState();
  m_pSceneDocument->m_LayerEvents.AddEventHandler(ezMakeDelegate(&ezLayerAction::LayerEventHandler, this));
  if (m_Type == ActionType::SaveActiveLayer)
  {
    m_pSceneDocument->s_EventsAny.AddEventHandler(ezMakeDelegate(&ezLayerAction::DocumentEventHandler, this));
  }
}


ezLayerAction::~ezLayerAction()
{
  m_pSceneDocument->m_LayerEvents.RemoveEventHandler(ezMakeDelegate(&ezLayerAction::LayerEventHandler, this));
  if (m_Type == ActionType::SaveActiveLayer)
  {
    m_pSceneDocument->s_EventsAny.RemoveEventHandler(ezMakeDelegate(&ezLayerAction::DocumentEventHandler, this));
  }
}

void ezLayerAction::ToggleLayerLoaded(ezScene2Document* pSceneDocument, ezUuid layerGuid)
{
  bool bLoad = !pSceneDocument->IsLayerLoaded(layerGuid);
  if (!bLoad)
  {
    ezSceneDocument* pLayer = pSceneDocument->GetLayerDocument(layerGuid);
    if (pLayer && pLayer->IsModified())
    {
      ezStringBuilder sMsg;
      ezStringBuilder sLayerName = "<Unknown>";
      {
        const ezAssetCurator::ezLockedSubAsset subAsset = ezAssetCurator::GetSingleton()->GetSubAsset(layerGuid);
        if (subAsset.isValid())
        {
          sLayerName = subAsset->GetName();
        }
      }
      sMsg.SetFormat("The layer '{}' has been modified.\nSave before unloading?", sLayerName);
      QMessageBox::StandardButton res = ezQtUiServices::MessageBoxQuestion(sMsg, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::No);
      switch (res)
      {
        case QMessageBox::Yes:
        {
          ezStatus saveRes = pLayer->SaveDocument();
          if (saveRes.Failed())
          {
            saveRes.LogFailure();
            return;
          }
        }
        break;
        case QMessageBox::Cancel:
          return;
        case QMessageBox::Default:
          break;
        default:
          break;
      }
    }
  }

  pSceneDocument->SetLayerLoaded(layerGuid, bLoad).LogFailure();

  if (bLoad)
  {
    pSceneDocument->SetActiveLayer(layerGuid).LogFailure();
  }
}

void ezLayerAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
    case ActionType::CreateLayer:
    {
      ezUuid layerGuid;
      QString name = QInputDialog::getText(GetContext().m_pWindow, "Add Layer", "Layer Name:");
      name = name.trimmed();
      if (name.isEmpty())
        return;
      ezStatus res = m_pSceneDocument->CreateLayer(name.toUtf8().data(), layerGuid);
      res.LogFailure();
      return;
    }
    case ActionType::DeleteLayer:
    {
      ezUuid layerGuid = GetCurrentSelectedLayer();
      m_pSceneDocument->DeleteLayer(layerGuid).LogFailure();
      return;
    }
    case ActionType::SaveLayer:
    {
      ezUuid layerGuid = GetCurrentSelectedLayer();
      if (ezSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(layerGuid))
      {
        pLayer->SaveDocument().LogFailure();
      }
      return;
    }
    case ActionType::SaveActiveLayer:
    {
      ezUuid layerGuid = m_pSceneDocument->GetActiveLayer();
      if (ezSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(layerGuid))
      {
        pLayer->SaveDocument().LogFailure();
      }
      return;
    }
    case ActionType::LayerLoaded:
    {
      ezUuid layerGuid = GetCurrentSelectedLayer();
      ToggleLayerLoaded(m_pSceneDocument, layerGuid);
      return;
    }
    case ActionType::LayerVisible:
    {
      ezUuid layerGuid = GetCurrentSelectedLayer();
      bool bVisible = !m_pSceneDocument->IsLayerVisible(layerGuid);
      m_pSceneDocument->SetLayerVisible(layerGuid, bVisible).LogFailure();
      return;
    }
  }
}

void ezLayerAction::LayerEventHandler(const ezScene2LayerEvent& e)
{
  UpdateEnableState();
}

void ezLayerAction::DocumentEventHandler(const ezDocumentEvent& e)
{
  UpdateEnableState();
}

void ezLayerAction::UpdateEnableState()
{
  ezUuid layerGuid = GetCurrentSelectedLayer();

  switch (m_Type)
  {
    case ActionType::CreateLayer:
      return;
    case ActionType::DeleteLayer:
    {
      SetEnabled(layerGuid.IsValid() && layerGuid != m_pSceneDocument->GetGuid());
      return;
    }
    case ActionType::SaveLayer:
    {
      ezSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(layerGuid);
      SetEnabled(pLayer && pLayer->IsModified());
      return;
    }
    case ActionType::SaveActiveLayer:
    {
      ezSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(m_pSceneDocument->GetActiveLayer());
      SetEnabled(pLayer && pLayer->IsModified());
      return;
    }
    case ActionType::LayerLoaded:
    {
      SetEnabled(layerGuid.IsValid() && layerGuid != m_pSceneDocument->GetGuid());
      SetChecked(m_pSceneDocument->IsLayerLoaded(layerGuid));
      return;
    }
    case ActionType::LayerVisible:
    {
      SetEnabled(layerGuid.IsValid());
      SetChecked(m_pSceneDocument->IsLayerVisible(layerGuid));
      return;
    }
  }
}

ezUuid ezLayerAction::GetCurrentSelectedLayer() const
{
  ezSelectionManager* pSelection = m_pSceneDocument->GetLayerSelectionManager();
  ezUuid layerGuid;
  if (const ezDocumentObject* pObject = pSelection->GetCurrentObject())
  {
    ezObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
    if (pObject->GetType()->IsDerivedFrom(ezGetStaticRTTI<ezSceneLayer>()))
    {
      layerGuid = pAccessor->GetByName<ezUuid>(pObject, "Layer");
    }
  }
  return layerGuid;
}
