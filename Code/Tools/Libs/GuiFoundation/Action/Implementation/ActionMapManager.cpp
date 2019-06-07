#include <GuiFoundationPCH.h>

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

ezResult ezActionMapManager::RegisterActionMap(const char* szMapping)
{
  auto it = s_Mappings.Find(szMapping);
  if (it.IsValid())
    return EZ_FAILURE;

  s_Mappings.Insert(szMapping, EZ_DEFAULT_NEW(ezActionMap));
  return EZ_SUCCESS;
}

ezResult ezActionMapManager::UnregisterActionMap(const char* szMapping)
{
  auto it = s_Mappings.Find(szMapping);
  if (!it.IsValid())
    return EZ_FAILURE;

  EZ_DEFAULT_DELETE(it.Value());
  s_Mappings.Remove(it);
  return EZ_SUCCESS;
}

ezActionMap* ezActionMapManager::GetActionMap(const char* szMapping)
{
  auto it = s_Mappings.Find(szMapping);
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
  ezDocumentActions::MapActions("DocumentWindowTabMenu", "", false);
}

void ezActionMapManager::Shutdown()
{
  ezActionMapManager::UnregisterActionMap("DocumentWindowTabMenu");

  while (!s_Mappings.IsEmpty())
  {
    ezResult res = UnregisterActionMap(s_Mappings.GetIterator().Key());
    EZ_ASSERT_DEV(res == EZ_SUCCESS, "Failed to call UnregisterActionMap successfully!");
  }
}
