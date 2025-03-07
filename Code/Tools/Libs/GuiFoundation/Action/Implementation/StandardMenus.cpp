#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

ezActionDescriptorHandle ezStandardMenus::s_hMenuProject;
ezActionDescriptorHandle ezStandardMenus::s_hMenuFile;
ezActionDescriptorHandle ezStandardMenus::s_hMenuEdit;
ezActionDescriptorHandle ezStandardMenus::s_hMenuPanels;
ezActionDescriptorHandle ezStandardMenus::s_hMenuScene;
ezActionDescriptorHandle ezStandardMenus::s_hMenuAsset;
ezActionDescriptorHandle ezStandardMenus::s_hMenuView;
ezActionDescriptorHandle ezStandardMenus::s_hMenuTools;
ezActionDescriptorHandle ezStandardMenus::s_hMenuHelp;
ezActionDescriptorHandle ezStandardMenus::s_hCheckForUpdates;
ezActionDescriptorHandle ezStandardMenus::s_hReportProblem;

void ezStandardMenus::RegisterActions()
{
  s_hMenuProject = EZ_REGISTER_MENU("G.Project");
  s_hMenuFile = EZ_REGISTER_MENU("G.File");
  s_hMenuEdit = EZ_REGISTER_MENU("G.Edit");
  s_hMenuPanels = EZ_REGISTER_DYNAMIC_MENU("G.Panels", ezApplicationPanelsMenuAction, "");
  s_hMenuScene = EZ_REGISTER_MENU("G.Scene");
  s_hMenuAsset = EZ_REGISTER_MENU("G.Asset");
  s_hMenuView = EZ_REGISTER_MENU("G.View");
  s_hMenuTools = EZ_REGISTER_MENU("G.Tools");
  s_hMenuHelp = EZ_REGISTER_MENU("G.Help");
  s_hCheckForUpdates = EZ_REGISTER_ACTION_1("Help.CheckForUpdates", ezActionScope::Global, "Help", "", ezHelpActions, ezHelpActions::ButtonType::CheckForUpdates);
  s_hReportProblem = EZ_REGISTER_ACTION_1("Help.ReportProblem", ezActionScope::Global, "Help", "", ezHelpActions, ezHelpActions::ButtonType::ReportProblem);
}

void ezStandardMenus::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hMenuProject);
  ezActionManager::UnregisterAction(s_hMenuFile);
  ezActionManager::UnregisterAction(s_hMenuEdit);
  ezActionManager::UnregisterAction(s_hMenuPanels);
  ezActionManager::UnregisterAction(s_hMenuScene);
  ezActionManager::UnregisterAction(s_hMenuAsset);
  ezActionManager::UnregisterAction(s_hMenuView);
  ezActionManager::UnregisterAction(s_hMenuTools);
  ezActionManager::UnregisterAction(s_hMenuHelp);
  ezActionManager::UnregisterAction(s_hCheckForUpdates);
  ezActionManager::UnregisterAction(s_hReportProblem);
}

void ezStandardMenus::MapActions(ezStringView sMapping, const ezBitflags<ezStandardMenuTypes>& menus)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "'{0}' does not exist", sMapping);

  ezActionMapDescriptor md;

  if (menus.IsAnySet(ezStandardMenuTypes::Project))
    pMap->MapAction(s_hMenuProject, "", -10000.0f);

  if (menus.IsAnySet(ezStandardMenuTypes::File))
    pMap->MapAction(s_hMenuFile, "", 1.0f);

  if (menus.IsAnySet(ezStandardMenuTypes::Edit))
    pMap->MapAction(s_hMenuEdit, "", 2.0f);

  if (menus.IsAnySet(ezStandardMenuTypes::Scene))
    pMap->MapAction(s_hMenuScene, "", 3.0f);

  if (menus.IsAnySet(ezStandardMenuTypes::Asset))
    pMap->MapAction(s_hMenuAsset, "", 4.0f);

  if (menus.IsAnySet(ezStandardMenuTypes::View))
    pMap->MapAction(s_hMenuView, "", 5.0f);

  if (menus.IsAnySet(ezStandardMenuTypes::Tools))
    pMap->MapAction(s_hMenuTools, "", 6.0f);

  if (menus.IsAnySet(ezStandardMenuTypes::Panels))
    pMap->MapAction(s_hMenuPanels, "", 7.0f);

  if (menus.IsAnySet(ezStandardMenuTypes::Help))
  {
    pMap->MapAction(s_hMenuHelp, "", 8.0f);
    pMap->MapAction(s_hReportProblem, "G.Help", 3.0f);
    pMap->MapAction(s_hCheckForUpdates, "G.Help", 10.0f);
  }
}

////////////////////////////////////////////////////////////////////////
// ezApplicationPanelsMenuAction
////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezApplicationPanelsMenuAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

struct ezComparePanels
{
  /// \brief Returns true if a is less than b
  EZ_ALWAYS_INLINE bool Less(const ezDynamicMenuAction::Item& p1, const ezDynamicMenuAction::Item& p2) const { return p1.m_sDisplay < p2.m_sDisplay; }

  /// \brief Returns true if a is equal to b
  EZ_ALWAYS_INLINE bool Equal(const ezDynamicMenuAction::Item& p1, const ezDynamicMenuAction::Item& p2) const
  {
    return p1.m_sDisplay == p2.m_sDisplay;
  }
};


void ezApplicationPanelsMenuAction::GetEntries(ezDynamicArray<Item>& out_entries)
{
  out_entries.Clear();

  for (auto* pPanel : ezQtApplicationPanel::GetAllApplicationPanels())
  {
    ezDynamicMenuAction::Item item;
    item.m_sDisplay = pPanel->windowTitle().toUtf8().data();
    item.m_UserValue = pPanel;
    item.m_Icon = pPanel->icon();
    item.m_CheckState = pPanel->isClosed() ? ezDynamicMenuAction::Item::CheckMark::Unchecked : ezDynamicMenuAction::Item::CheckMark::Checked;

    out_entries.PushBack(item);
  }

  // make sure the panels appear in alphabetical order in the menu
  ezComparePanels cp;
  out_entries.Sort<ezComparePanels>(cp);
}

void ezApplicationPanelsMenuAction::Execute(const ezVariant& value)
{
  ezQtApplicationPanel* pPanel = static_cast<ezQtApplicationPanel*>(value.ConvertTo<void*>());
  if (pPanel->isClosed())
  {
    pPanel->toggleView(true);
    pPanel->EnsureVisible();
  }
  else
  {
    pPanel->toggleView(false);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezHelpActions, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezHelpActions::ezHelpActions(const ezActionContext& context, const char* szName, ButtonType button)
  : ezButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  if (button == ButtonType::ReportProblem)
  {
    SetIconPath(":/EditorFramework/Icons/GitHub-Light.svg");
  }
}

ezHelpActions::~ezHelpActions() = default;

void ezHelpActions::Execute(const ezVariant& value)
{
  if (m_ButtonType == ButtonType::ReportProblem)
  {
    QDesktopServices::openUrl(QUrl("https://github.com/ezEngine/ezEngine/issues"));
  }
  if (m_ButtonType == ButtonType::CheckForUpdates)
  {
    ezQtUiServices::CheckForUpdates();
  }
}
