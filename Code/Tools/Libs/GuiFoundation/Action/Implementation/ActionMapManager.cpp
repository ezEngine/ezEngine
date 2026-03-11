#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/DocumentActions.h>

ezMap<ezString, ezActionMap*> ezActionMapManager::s_Mappings;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, ActionMapManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ActionManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezActionMapManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezActionMapManager::Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

////////////////////////////////////////////////////////////////////////
// ezActionMapManager public functions
////////////////////////////////////////////////////////////////////////

void ezActionMapManager::RegisterActionMap(ezStringView sActionMapName, ezStringView sParentActionMapName)
{
  auto it = s_Mappings.Find(sActionMapName);
  EZ_ASSERT_ALWAYS(!it.IsValid(), "Mapping '{}' already exists", sActionMapName);
  s_Mappings.Insert(sActionMapName, EZ_DEFAULT_NEW(ezActionMap, sParentActionMapName));
}

void ezActionMapManager::UnregisterActionMap(ezStringView sActionMapName)
{
  auto it = s_Mappings.Find(sActionMapName);
  EZ_ASSERT_ALWAYS(it.IsValid(), "Mapping '{}' not found", sActionMapName);
  EZ_DEFAULT_DELETE(it.Value());
  s_Mappings.Remove(it);
}

ezActionMap* ezActionMapManager::GetActionMap(ezStringView sActionMapName)
{
  auto it = s_Mappings.Find(sActionMapName);
  if (!it.IsValid())
    return nullptr;

  return it.Value();
}


////////////////////////////////////////////////////////////////////////
// ezActionMapManager private functions
////////////////////////////////////////////////////////////////////////

void ezActionMapManager::Startup()
{
  ezActionMapManager::RegisterActionMap("DocumentWindowTabMenu");
  ezDocumentActions::MapMenuActions("DocumentWindowTabMenu", "");
}

void ezActionMapManager::Shutdown()
{
  ezActionMapManager::UnregisterActionMap("DocumentWindowTabMenu");

  while (!s_Mappings.IsEmpty())
  {
    UnregisterActionMap(s_Mappings.GetIterator().Key());
  }
}
