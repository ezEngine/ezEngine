#include <EditorPluginScenePCH.h>

#include <EditorPluginScene/Actions/LayerActions.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <QInputDialog>


// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLayerAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActionDescriptorHandle ezLayerActions::s_hLayerCategory;
ezActionDescriptorHandle ezLayerActions::s_hCreateLayer;
ezActionDescriptorHandle ezLayerActions::s_hDeleteLayer;
ezActionDescriptorHandle ezLayerActions::s_hLayerLoaded;
ezActionDescriptorHandle ezLayerActions::s_hLayerVisible;

void ezLayerActions::RegisterActions()
{
  s_hLayerCategory = EZ_REGISTER_CATEGORY("LayerCategory");

  s_hCreateLayer = EZ_REGISTER_ACTION_1("Layer.CreateLayer", ezActionScope::Document, "Scene - Layer", "",
    ezLayerAction, ezLayerAction::ActionType::CreateLayer);
  s_hDeleteLayer = EZ_REGISTER_ACTION_1("Layer.DeleteLayer", ezActionScope::Document, "Scene - Layer", "",
    ezLayerAction, ezLayerAction::ActionType::DeleteLayer);
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
  ezActionManager::UnregisterAction(s_hLayerLoaded);
  ezActionManager::UnregisterAction(s_hLayerVisible);
}

void ezLayerActions::MapContextMenuActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);


  pMap->MapAction(s_hLayerCategory, "", 0.0f);
  ezStringBuilder sSubPath(szPath, "/LayerCategory");
  pMap->MapAction(s_hCreateLayer, sSubPath, 1.0f);
  pMap->MapAction(s_hDeleteLayer, sSubPath, 2.0f);
  pMap->MapAction(s_hLayerLoaded, sSubPath, 3.0f);
  pMap->MapAction(s_hLayerVisible, sSubPath, 4.0f);
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
      SetIconPath(":/EditorPluginScene/Icons/CreateNode16.png");
      break;
    case ActionType::DeleteLayer:
      SetIconPath(":/EditorPluginScene/Icons/CreateNode16.png");
      break;
    case ActionType::LayerLoaded:
      SetCheckable(true);
      SetIconPath(":/EditorPluginScene/Icons/CreateNode16.png");
      break;
    case ActionType::LayerVisible:
      SetCheckable(true);
      SetIconPath(":/EditorPluginScene/Icons/CreateNode16.png");
      break;

  }

  UpdateEnableState();

  m_pSceneDocument->m_LayerEvents.AddEventHandler(ezMakeDelegate(&ezLayerAction::LayerEventHandler, this));
}


ezLayerAction::~ezLayerAction()
{
  m_pSceneDocument->m_LayerEvents.RemoveEventHandler(ezMakeDelegate(&ezLayerAction::LayerEventHandler, this));
}

void ezLayerAction::Execute(const ezVariant& value)
{
  ezUuid layerGuid = GetCurrentSelectedLayer();
  
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
    case ActionType::LayerLoaded:
    {
      ezUuid layerGuid = GetCurrentSelectedLayer();
      m_pSceneDocument->SetLayerLoaded(layerGuid, !m_pSceneDocument->IsLayerLoaded(layerGuid)).LogFailure();
      return;
    }
    case ActionType::LayerVisible:
    {
      /*ezUuid layerGuid = GetCurrentSelectedLayer();
      m_pSceneDocument->SetLayerLoaded(layerGuid, !m_pSceneDocument->IsLayerLoaded(layerGuid)).LogFailure();*/
      return;
    }
  }
}

void ezLayerAction::LayerEventHandler(const ezScene2LayerEvent& e)
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
    case ActionType::LayerLoaded:
    {
      SetEnabled(layerGuid.IsValid() && layerGuid != m_pSceneDocument->GetGuid());
      SetChecked(m_pSceneDocument->IsLayerLoaded(layerGuid));
      return;
    }
    case ActionType::LayerVisible:
    {
      SetEnabled(layerGuid.IsValid() && layerGuid != m_pSceneDocument->GetGuid());
      SetChecked(true);
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
      layerGuid = pAccessor->Get<ezUuid>(pObject, "Layer");
    }
  }
  return layerGuid;
}
