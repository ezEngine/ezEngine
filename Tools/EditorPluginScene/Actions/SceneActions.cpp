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


void ezSceneActions::MapMenuActions()
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap("EditorPluginScene_DocumentMenuBar");
  EZ_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    const char* szSubPath = "Menu.Tools/SceneCategory";

    pMap->MapAction(s_hSceneCategory, "Menu.Tools", 6.0f);
    pMap->MapAction(s_hUpdatePrefabs, szSubPath, 1.0f);
  }

  {
    const char* szSubPath = "Menu.Scene/SceneCategory";

    pMap->MapAction(s_hSceneCategory, "Menu.Scene", 1.0f);
    pMap->MapAction(s_hExportScene, szSubPath, 1.0f);
    pMap->MapAction(s_hRunScene, szSubPath, 2.0f);
    pMap->MapAction(s_hEnableWorldSimulation, szSubPath, 3.0f);
  }

  {
    const char* szSubPath = "Menu.View/SceneCategory";

    pMap->MapAction(s_hSceneCategory, "Menu.View", 1.0f);
    pMap->MapAction(s_hRenderSelectionOverlay, szSubPath, 1.0f);
  }
}


void ezSceneActions::MapToolbarActions()
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap("EditorPluginScene_DocumentToolBar");
  EZ_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    const char* szSubPath = "SceneCategory";

    /// \todo This works incorrectly with value 6.0f -> it places the action inside the snap category
    pMap->MapAction(s_hSceneCategory, "", 7.0f);

    pMap->MapAction(s_hEnableWorldSimulation, szSubPath, 1.0f);
    pMap->MapAction(s_hRenderSelectionOverlay, szSubPath, 2.0f);
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
    SetIconPath(":/EditorPluginScene/PrefabUpdate.png");
    break;
  case ActionType::ExportScene:
    SetIconPath(":/EditorPluginScene/Icons/SceneExport16.png");
    break;
  case ActionType::RunScene:
    SetIconPath(":/EditorPluginScene/Icons/SceneRun16.png");
    break;
  case ActionType::SimulateWorld:
    SetCheckable(true);
    SetIconPath(":/EditorPluginScene/Icons/ScenePlay16.png");
    SetChecked(m_pSceneDocument->GetSimulateWorld());
    break;
  case ActionType::RenderSelectionOverlay:
    SetCheckable(true);
    SetIconPath(":/EditorPluginScene/Icons/Selection16.png");
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

