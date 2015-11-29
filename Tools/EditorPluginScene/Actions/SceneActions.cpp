#include <PCH.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorPluginScene/Actions/SceneActions.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <QProcess>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezActionDescriptorHandle ezSceneActions::s_hSceneCategory;
ezActionDescriptorHandle ezSceneActions::s_hUpdatePrefabs;
ezActionDescriptorHandle ezSceneActions::s_hExportScene;
ezActionDescriptorHandle ezSceneActions::s_hRunScene;
ezActionDescriptorHandle ezSceneActions::s_hEnableWorldSimulation;
ezActionDescriptorHandle ezSceneActions::s_hRenderSelectionOverlay;

void ezSceneActions::RegisterActions()
{
  s_hSceneCategory = EZ_REGISTER_CATEGORY("SceneCategory");
  s_hUpdatePrefabs = EZ_REGISTER_ACTION_1("Prefabs.UpdateAll", ezActionScope::Document, "Scene", "Ctrl+Shift+P", ezSceneAction, ezSceneAction::ActionType::UpdatePrefabs);
  s_hExportScene = EZ_REGISTER_ACTION_1("Scene.Export", ezActionScope::Document, "Scene", "Ctrl+E", ezSceneAction, ezSceneAction::ActionType::ExportScene);
  s_hRunScene = EZ_REGISTER_ACTION_1("Scene.Run", ezActionScope::Document, "Scene", "Ctrl+R", ezSceneAction, ezSceneAction::ActionType::RunScene);
  s_hEnableWorldSimulation = EZ_REGISTER_ACTION_1("Scene.SimulateWorld", ezActionScope::Document, "Scene", "Ctrl+F5", ezSceneAction, ezSceneAction::ActionType::SimulateWorld);
  s_hRenderSelectionOverlay = EZ_REGISTER_ACTION_1( "Scene.Render.SelectionOverlay", ezActionScope::Document, "Scene", "Ctrl+M", ezSceneAction, ezSceneAction::ActionType::RenderSelectionOverlay);
}

void ezSceneActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hSceneCategory);
  ezActionManager::UnregisterAction(s_hUpdatePrefabs);
  ezActionManager::UnregisterAction(s_hExportScene);
  ezActionManager::UnregisterAction(s_hRunScene);
  ezActionManager::UnregisterAction(s_hEnableWorldSimulation);
  ezActionManager::UnregisterAction(s_hRenderSelectionOverlay);
}

void ezSceneActions::MapActions(const char* szMapping, const char* szPath, bool bToolbar)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/SceneCategory");

  if (bToolbar)
  {
    /// \todo This works incorrectly with value 6.0f -> it places the action inside the snap category
    pMap->MapAction(s_hSceneCategory, szPath, 7.0f);

    pMap->MapAction(s_hEnableWorldSimulation, sSubPath, 1.0f);
    pMap->MapAction(s_hRenderSelectionOverlay, sSubPath, 2.0f);
  }
  else
  {
    pMap->MapAction(s_hSceneCategory, szPath, 6.0f);

    pMap->MapAction(s_hUpdatePrefabs, sSubPath, 1.0f);
    pMap->MapAction(s_hExportScene, sSubPath, 2.0f);
    pMap->MapAction(s_hRunScene, sSubPath, 3.0f);
    pMap->MapAction(s_hEnableWorldSimulation, sSubPath, 4.0f);
	pMap->MapAction(s_hRenderSelectionOverlay, sSubPath, 4.0f);
  }
}


ezSceneAction::ezSceneAction(const ezActionContext& context, const char* szName, ezSceneAction::ActionType type) : ezButtonAction(context, szName, false, "")
{
  m_Type = type;
  m_pSceneDocument = static_cast<ezSceneDocument*>(context.m_pDocument);
  m_pSceneDocument->m_SceneEvents.AddEventHandler(ezMakeDelegate(&ezSceneAction::SceneEventHandler, this));

  switch (m_Type)
  {
  case ActionType::UpdatePrefabs:
    SetIconPath(":/AssetIcons/PrefabUpdate.png");
    break;
  case ActionType::ExportScene:
    SetIconPath(":/GuiFoundation/Icons/SceneExport16.png");
    break;
  case ActionType::RunScene:
    SetIconPath(":/GuiFoundation/Icons/SceneRun16.png");
    break;
  case ActionType::SimulateWorld:
    SetCheckable(true);
    SetIconPath(":/GuiFoundation/Icons/ScenePlay16.png");
    SetChecked(m_pSceneDocument->GetSimulateWorld());
    break;
  case ActionType::RenderSelectionOverlay:
    SetCheckable(true);
    SetIconPath(":/GuiFoundation/Icons/Selection16.png");
    SetChecked(m_pSceneDocument->GetRenderSelectionOverlay());
    break;
  }
}

ezSceneAction::~ezSceneAction()
{
  m_pSceneDocument->m_SceneEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneAction::SceneEventHandler, this));
}

void ezSceneAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
  case ActionType::UpdatePrefabs:
    m_pSceneDocument->UpdatePrefabs();
    return;
  case ActionType::ExportScene:
    m_pSceneDocument->TriggerExportScene();
    return;
  case ActionType::RunScene:
    {
      QStringList arguments;
      arguments << "-scene";
      arguments << QString::fromUtf8(m_pSceneDocument->GetBinaryTargetFile().GetData());

      QProcess proc;
      proc.startDetached(QString::fromUtf8("Player.exe"), arguments);
    }
    return;

  case ActionType::SimulateWorld:
    {
      m_pSceneDocument->SetSimulateWorld(!m_pSceneDocument->GetSimulateWorld());
    }
    return;

  case ActionType::RenderSelectionOverlay:
    {
      m_pSceneDocument->SetRenderSelectionOverlay(!m_pSceneDocument->GetRenderSelectionOverlay());
    }
    return;
  }
}

void ezSceneAction::SceneEventHandler(const ezSceneDocument::SceneEvent& e)
{
  switch (e.m_Type)
  {
  case ezSceneDocument::SceneEvent::Type::SimulateModeChanged:
    {
      if (m_Type == ActionType::SimulateWorld)
      {
        SetChecked(m_pSceneDocument->GetSimulateWorld());
      }
    }
    break;

  case ezSceneDocument::SceneEvent::Type::RenderSelectionOverlayChanged:
    {
      if (m_Type == ActionType::RenderSelectionOverlay)
      {
        SetChecked(m_pSceneDocument->GetRenderSelectionOverlay());
      }
    }
    break;
  }
}

