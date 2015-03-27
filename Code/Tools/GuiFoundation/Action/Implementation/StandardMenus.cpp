#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

ezActionDescriptorHandle ezStandardMenus::s_hMenuFile;
ezActionDescriptorHandle ezStandardMenus::s_hMenuEdit;
ezActionDescriptorHandle ezStandardMenus::s_hMenuView;
ezActionDescriptorHandle ezStandardMenus::s_hMenuSettings;
ezActionDescriptorHandle ezStandardMenus::s_hMenuHelp;

void ezStandardMenus::RegisterActions()
{
  s_hMenuFile = EZ_REGISTER_MENU("File", "File");
  s_hMenuEdit = EZ_REGISTER_MENU("Edit", "Edit");
  s_hMenuView = EZ_REGISTER_MENU("View", "View");
  s_hMenuSettings = EZ_REGISTER_MENU("Settings", "Settings");
  s_hMenuHelp = EZ_REGISTER_MENU("Help", "Help");

}

void ezStandardMenus::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hMenuFile);
  ezActionManager::UnregisterAction(s_hMenuEdit);
  ezActionManager::UnregisterAction(s_hMenuView);
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

  if (Menus.IsAnySet(ezStandardMenuTypes::View))
    pMap->MapAction(s_hMenuView, "", 3.0f);

  if (Menus.IsAnySet(ezStandardMenuTypes::Settings))
    pMap->MapAction(s_hMenuSettings, "", 4.0f);

  if (Menus.IsAnySet(ezStandardMenuTypes::Help))
    pMap->MapAction(s_hMenuHelp, "", 5.0f);
}

