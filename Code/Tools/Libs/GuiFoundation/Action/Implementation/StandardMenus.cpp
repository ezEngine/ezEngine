#include <GuiFoundationPCH.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>

ezActionDescriptorHandle ezStandardMenus::s_hMenuFile;
ezActionDescriptorHandle ezStandardMenus::s_hMenuEdit;
ezActionDescriptorHandle ezStandardMenus::s_hMenuPanels;
ezActionDescriptorHandle ezStandardMenus::s_hMenuProject;
ezActionDescriptorHandle ezStandardMenus::s_hMenuScene;
ezActionDescriptorHandle ezStandardMenus::s_hMenuView;
ezActionDescriptorHandle ezStandardMenus::s_hMenuHelp;

void ezStandardMenus::RegisterActions()
{
  s_hMenuFile = EZ_REGISTER_MENU("Menu.File");
  s_hMenuEdit = EZ_REGISTER_MENU("Menu.Edit");
  s_hMenuPanels = EZ_REGISTER_DYNAMIC_MENU("Menu.Panels", ezApplicationPanelsMenuAction, "");
  s_hMenuProject = EZ_REGISTER_MENU("Menu.Project");
  s_hMenuScene = EZ_REGISTER_MENU("Menu.Scene");
  s_hMenuView = EZ_REGISTER_MENU("Menu.View");
  s_hMenuHelp = EZ_REGISTER_MENU("Menu.Help");
}

void ezStandardMenus::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hMenuFile);
  ezActionManager::UnregisterAction(s_hMenuEdit);
  ezActionManager::UnregisterAction(s_hMenuPanels);
  ezActionManager::UnregisterAction(s_hMenuProject);
  ezActionManager::UnregisterAction(s_hMenuScene);
  ezActionManager::UnregisterAction(s_hMenuView);
  ezActionManager::UnregisterAction(s_hMenuHelp);
}

void ezStandardMenus::MapActions(const char* szMapping, const ezBitflags<ezStandardMenuTypes>& Menus)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "'{0}' does not exist", szMapping);

  ezActionMapDescriptor md;

  if (Menus.IsAnySet(ezStandardMenuTypes::File))
    pMap->MapAction(s_hMenuFile, "", 1.0f);

  if (Menus.IsAnySet(ezStandardMenuTypes::Edit))
    pMap->MapAction(s_hMenuEdit, "", 2.0f);

  if (Menus.IsAnySet(ezStandardMenuTypes::Project))
    pMap->MapAction(s_hMenuProject, "", 3.0f);

  if (Menus.IsAnySet(ezStandardMenuTypes::Scene))
    pMap->MapAction(s_hMenuScene, "", 4.0f);

  if (Menus.IsAnySet(ezStandardMenuTypes::View))
    pMap->MapAction(s_hMenuView, "", 5.0f);

  if (Menus.IsAnySet(ezStandardMenuTypes::Panels))
    pMap->MapAction(s_hMenuPanels, "", 6.0f);

  // not used at the moment
  // if (Menus.IsAnySet(ezStandardMenuTypes::Help))
  // pMap->MapAction(s_hMenuHelp, "", 7.0f);
}

////////////////////////////////////////////////////////////////////////
// ezApplicationPanelsMenuAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezApplicationPanelsMenuAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

struct ezComparePanels
{
  /// \brief Returns true if a is less than b
  EZ_ALWAYS_INLINE bool Less(const ezDynamicMenuAction::Item& p1, const ezDynamicMenuAction::Item& p2) const
  {
    return p1.m_sDisplay < p2.m_sDisplay;
  }

  /// \brief Returns true if a is equal to b
  EZ_ALWAYS_INLINE bool Equal(const ezDynamicMenuAction::Item& p1, const ezDynamicMenuAction::Item& p2) const
  {
    return p1.m_sDisplay == p2.m_sDisplay;
  }
};


void ezApplicationPanelsMenuAction::GetEntries(ezHybridArray<ezDynamicMenuAction::Item, 16>& out_Entries)
{
  out_Entries.Clear();

  for (auto* pPanel : ezQtApplicationPanel::GetAllApplicationPanels())
  {
    ezDynamicMenuAction::Item item;
    item.m_sDisplay = pPanel->windowTitle().toUtf8().data();
    item.m_UserValue = pPanel;
    item.m_Icon = pPanel->windowIcon();
    item.m_CheckState =
        pPanel->isVisible() ? ezDynamicMenuAction::Item::CheckMark::Checked : ezDynamicMenuAction::Item::CheckMark::Unchecked;

    out_Entries.PushBack(item);
  }

  // make sure the panels appear in alphabetical order in the menu
  ezComparePanels cp;
  out_Entries.Sort<ezComparePanels>(cp);
}

void ezApplicationPanelsMenuAction::Execute(const ezVariant& value)
{
  ezQtApplicationPanel* pPanel = static_cast<ezQtApplicationPanel*>(value.ConvertTo<void*>());

  if (pPanel->isVisible())
    pPanel->close();
  else
    pPanel->EnsureVisible();
}
