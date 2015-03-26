#include <PCH.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <EditorFramework/Settings/SettingsTab.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <Foundation/IO/OSFile.h>

ezActionDescriptorHandle ezProjectActions::s_hEditorMenu;
ezActionDescriptorHandle ezProjectActions::s_hCreateDocument;
ezActionDescriptorHandle ezProjectActions::s_hOpenDocument;
ezActionDescriptorHandle ezProjectActions::s_hRecentDocuments;
ezActionDescriptorHandle ezProjectActions::s_hCreateProject;
ezActionDescriptorHandle ezProjectActions::s_hOpenProject;
ezActionDescriptorHandle ezProjectActions::s_hRecentProjects;
ezActionDescriptorHandle ezProjectActions::s_hCloseProject;
ezActionDescriptorHandle ezProjectActions::s_hProjectSettings;

#define REGISTER_ACTION(name, type) \
  ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Action, ezActionScope::Global, name, sCategory,  \
    [](const ezActionContext& context)->ezAction* { return EZ_DEFAULT_NEW(ezProjectAction)(context, name, type); }));

#define REGISTER_LRU(name, type) \
  ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Menu, ezActionScope::Global, name, sCategory,  \
    [](const ezActionContext& context)->ezAction* { return EZ_DEFAULT_NEW(type)(context, name); }));

void ezProjectActions::RegisterActions()
{
  ezHashedString sCategory;
  sCategory.Assign("Project");

  s_hEditorMenu = ezActionManager::RegisterAction(ezActionDescriptor(ezActionType::Menu, ezActionScope::Global, "EditorMenu", sCategory,
    [](const ezActionContext& context)->ezAction* { return EZ_DEFAULT_NEW(ezMenuAction)(context, "Editor"); }));

  s_hCreateDocument = REGISTER_ACTION("Create Document", ezProjectAction::ButtonType::CreateDocument);
  s_hOpenDocument = REGISTER_ACTION("Open Document", ezProjectAction::ButtonType::OpenDocument);
  s_hRecentDocuments = REGISTER_LRU("Recent Documents", ezRecentDocumentsMenuAction);
  s_hCreateProject = REGISTER_ACTION("Create Project", ezProjectAction::ButtonType::CreateProject);
  s_hOpenProject = REGISTER_ACTION("Open Project", ezProjectAction::ButtonType::OpenProject);
  s_hRecentProjects = REGISTER_LRU("Recent Projects", ezRecentProjectsMenuAction);
  s_hCloseProject = REGISTER_ACTION("Close Project", ezProjectAction::ButtonType::CloseProject);
  s_hProjectSettings = REGISTER_ACTION("Settings", ezProjectAction::ButtonType::ProjectSettings);
}

void ezProjectActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hEditorMenu);
  ezActionManager::UnregisterAction(s_hCreateDocument);
  ezActionManager::UnregisterAction(s_hOpenDocument);
  ezActionManager::UnregisterAction(s_hRecentDocuments);
  ezActionManager::UnregisterAction(s_hCreateProject);
  ezActionManager::UnregisterAction(s_hOpenProject);
  ezActionManager::UnregisterAction(s_hRecentProjects);
  ezActionManager::UnregisterAction(s_hCloseProject);
  ezActionManager::UnregisterAction(s_hProjectSettings);

  s_hEditorMenu.Invalidate();
  s_hCreateDocument.Invalidate();
  s_hOpenDocument.Invalidate();
  s_hRecentDocuments.Invalidate();
  s_hCreateProject.Invalidate();
  s_hOpenProject.Invalidate();
  s_hRecentProjects.Invalidate();
  s_hCloseProject.Invalidate();
  s_hProjectSettings.Invalidate();
}

void ezProjectActions::MapActions(const char* szMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  ezActionMapDescriptor desc;

  desc.m_hAction = s_hEditorMenu;
  desc.m_fOrder = -1000000000.0f;
  EZ_VERIFY(pMap->MapAction(desc).IsValid(), "Mapping failed");

  desc.m_sPath.Assign("EditorMenu");

  desc.m_hAction = s_hCreateDocument;
  desc.m_fOrder = 1.0f;
  EZ_VERIFY(pMap->MapAction(desc).IsValid(), "Mapping failed");

  desc.m_hAction = s_hOpenDocument;
  desc.m_fOrder = 2.0f;
  EZ_VERIFY(pMap->MapAction(desc).IsValid(), "Mapping failed");

  desc.m_hAction = s_hRecentDocuments;
  desc.m_fOrder = 2.5f;
  EZ_VERIFY(pMap->MapAction(desc).IsValid(), "Mapping failed");

  desc.m_hAction = s_hCreateProject;
  desc.m_fOrder = 3.0f;
  EZ_VERIFY(pMap->MapAction(desc).IsValid(), "Mapping failed");

  desc.m_hAction = s_hOpenProject;
  desc.m_fOrder = 4.0f;
  EZ_VERIFY(pMap->MapAction(desc).IsValid(), "Mapping failed");

  desc.m_hAction = s_hRecentProjects;
  desc.m_fOrder = 4.5f;
  EZ_VERIFY(pMap->MapAction(desc).IsValid(), "Mapping failed");

  desc.m_hAction = s_hCloseProject;
  desc.m_fOrder = 5.0f;
  EZ_VERIFY(pMap->MapAction(desc).IsValid(), "Mapping failed");

  desc.m_hAction = s_hProjectSettings;
  desc.m_fOrder = 6.0f;
  EZ_VERIFY(pMap->MapAction(desc).IsValid(), "Mapping failed");

}

////////////////////////////////////////////////////////////////////////
// ezRecentDocumentsMenuAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRecentDocumentsMenuAction, ezLRUMenuAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();


void ezRecentDocumentsMenuAction::GetEntries(ezHybridArray<std::pair<ezString, ezVariant>, 16>& out_Entries)
{
  out_Entries.Clear();

  if (ezEditorApp::GetInstance()->GetRecentDocumentsList().GetFileList().IsEmpty())
    return;

  ezInt32 iMaxDocumentsToAdd = 10;
  for (ezString s : ezEditorApp::GetInstance()->GetRecentDocumentsList().GetFileList())
  {
    QAction* pAction = nullptr;

    if (!ezOSFile::Exists(s))
      continue;

    if (ezToolsProject::IsProjectOpen())
    {
      ezString sRelativePath;
      if (!ezToolsProject::GetInstance()->IsDocumentInProject(s, &sRelativePath))
        continue;

      out_Entries.PushBack(std::pair<ezString, ezVariant>(sRelativePath, s));
    }
    else
    {
      out_Entries.PushBack(std::pair<ezString, ezVariant>(s, s));
    }

    --iMaxDocumentsToAdd;

    if (iMaxDocumentsToAdd <= 0)
      break;
  }
}

void ezRecentDocumentsMenuAction::Execute(const ezVariant& value)
{
  ezContainerWindow::CreateOrOpenDocument(false, value.ConvertTo<ezString>());
}


////////////////////////////////////////////////////////////////////////
// ezRecentDocumentsMenuAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRecentProjectsMenuAction, ezLRUMenuAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();


void ezRecentProjectsMenuAction::GetEntries(ezHybridArray<std::pair<ezString, ezVariant>, 16>& out_Entries)
{
  out_Entries.Clear();

  if (ezEditorApp::GetInstance()->GetRecentProjectsList().GetFileList().IsEmpty())
    return;

  for (ezString s : ezEditorApp::GetInstance()->GetRecentProjectsList().GetFileList())
  {
    if (!ezOSFile::Exists(s))
      continue;

    out_Entries.PushBack(std::pair<ezString, ezVariant>(s, s));
  }
}

void ezRecentProjectsMenuAction::Execute(const ezVariant& value)
{
  ezContainerWindow::CreateOrOpenProject(false, value.ConvertTo<ezString>());
}

////////////////////////////////////////////////////////////////////////
// ezProjectAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProjectAction, ezButtonAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezProjectAction::ezProjectAction(const ezActionContext& context, const char* szName, ButtonType button)
  : ezButtonAction(context, szName, false)
{
  m_ButtonType = button;

  if (m_ButtonType == ButtonType::CloseProject)
  {
    SetEnabled(ezToolsProject::IsProjectOpen());

    ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezProjectAction::ProjectEventHandler, this));
  }
}

ezProjectAction::~ezProjectAction()
{
  if (m_ButtonType == ButtonType::CloseProject)
  {
    ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezProjectAction::ProjectEventHandler, this));
  }
}

void ezProjectAction::ProjectEventHandler(const ezToolsProject::Event& e)
{
  SetEnabled(ezToolsProject::IsProjectOpen());
  TriggerUpdate();
}

void ezProjectAction::Execute(const ezVariant& value)
{
  switch (m_ButtonType)
  {
  case ezProjectAction::ButtonType::CreateDocument:
    ezContainerWindow::CreateOrOpenDocument(true);
    break;

  case ezProjectAction::ButtonType::OpenDocument:
    ezContainerWindow::CreateOrOpenDocument(false);
    break;

  case ezProjectAction::ButtonType::CreateProject:
    ezContainerWindow::CreateOrOpenProject(true);
    break;

  case ezProjectAction::ButtonType::OpenProject:
    ezContainerWindow::CreateOrOpenProject(false);
    break;

  case ezProjectAction::ButtonType::CloseProject:
    {
      if (ezToolsProject::CanCloseProject())
        ezToolsProject::CloseProject();
    }
    break;

  case ezProjectAction::ButtonType::ProjectSettings:
    ezEditorApp::GetInstance()->ShowSettingsDocument();
    break;
  }

}

