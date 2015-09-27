#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>

ezActionDescriptorHandle ezStandardMenus::s_hMenuFile;
ezActionDescriptorHandle ezStandardMenus::s_hMenuEdit;
ezActionDescriptorHandle ezStandardMenus::s_hMenuPanels;
ezActionDescriptorHandle ezStandardMenus::s_hMenuSettings;
ezActionDescriptorHandle ezStandardMenus::s_hMenuHelp;

void ezStandardMenus::RegisterActions()
{
  s_hMenuFile = EZ_REGISTER_MENU("File", "File");
  s_hMenuEdit = EZ_REGISTER_MENU("Edit", "Edit");
  s_hMenuPanels = EZ_REGISTER_LRU_MENU("Panels", "Panels", ezApplicationPanelsMenuAction, "");
  s_hMenuSettings = EZ_REGISTER_MENU("Settings", "Settings");
  s_hMenuHelp = EZ_REGISTER_MENU("Help", "Help");

}

void ezStandardMenus::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hMenuFile);
  ezActionManager::UnregisterAction(s_hMenuEdit);
  ezActionManager::UnregisterAction(s_hMenuPanels);
  ezActionManager::UnregisterAction(s_hMenuSettings);
  ezActionManager::UnregisterAction(s_hMenuHelp);
}

void ezStandardMenus::MapActions(const char* szMapping, const ezBitflags<ezStandardMenuTypes>& Menus)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "'%s' does not exist", szMapping);

  ezActionMapDescriptor md;

  if (Menus.IsAnySet(ezStandardMenuTypes::File))
    pMap->MapAction(s_hMenuFile, "", 1.0f);

  if (Menus.IsAnySet(ezStandardMenuTypes::Edit))
    pMap->MapAction(s_hMenuEdit, "", 2.0f);

  if (Menus.IsAnySet(ezStandardMenuTypes::Panels))
    pMap->MapAction(s_hMenuPanels, "", 3.0f);

  if (Menus.IsAnySet(ezStandardMenuTypes::Settings))
    pMap->MapAction(s_hMenuSettings, "", 4.0f);

  if (Menus.IsAnySet(ezStandardMenuTypes::Help))
    pMap->MapAction(s_hMenuHelp, "", 5.0f);
}

////////////////////////////////////////////////////////////////////////
// ezApplicationPanelsMenuAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezApplicationPanelsMenuAction, ezLRUMenuAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

struct ezComparePanels
{
  /// \brief Returns true if a is less than b
  EZ_FORCE_INLINE bool Less(const ezLRUMenuAction::Item& p1, const ezLRUMenuAction::Item& p2) const
  {
    return p1.m_sDisplay < p2.m_sDisplay;
  }

  /// \brief Returns true if a is equal to b
  EZ_FORCE_INLINE bool Equal(const ezLRUMenuAction::Item& p1, const ezLRUMenuAction::Item& p2) const
  {
    return p1.m_sDisplay == p2.m_sDisplay;
  }
};


void ezApplicationPanelsMenuAction::GetEntries(ezHybridArray<ezLRUMenuAction::Item, 16>& out_Entries)
{
  out_Entries.Clear();

  for (auto* pPanel : ezApplicationPanel::GetAllApplicationPanels())
  {
    ezLRUMenuAction::Item item;
    item.m_sDisplay = pPanel->windowTitle().toUtf8().data();
    item.m_UserValue = pPanel;
    item.m_Icon = pPanel->windowIcon();
    item.m_CheckState = pPanel->isVisible() ? ezLRUMenuAction::Item::CheckMark::Checked : ezLRUMenuAction::Item::CheckMark::Unchecked;
    
    out_Entries.PushBack(item);
  }

  // make sure the panels appear in alphabetical order in the menu
  ezComparePanels cp;
  out_Entries.Sort<ezComparePanels>(cp);
}

void ezApplicationPanelsMenuAction::Execute(const ezVariant& value)
{
  ezApplicationPanel* pPanel = static_cast<ezApplicationPanel*>(value.ConvertTo<void*>());

  if (pPanel->isVisible())
    pPanel->close();
  else
    pPanel->EnsureVisible();
}
