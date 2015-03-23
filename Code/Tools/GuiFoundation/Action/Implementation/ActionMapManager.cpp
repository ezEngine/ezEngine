#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <Foundation/Configuration/Startup.h>

ezMap<ezHashedString, ezActionMap*> ezActionMapManager::s_Mappings;

EZ_BEGIN_SUBSYSTEM_DECLARATION(Core, ActionMapManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ActionManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezActionMapManager::Startup();
  }

  ON_CORE_SHUTDOWN
  {
    ezActionMapManager::Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION

////////////////////////////////////////////////////////////////////////
// ezActionMapManager public functions
////////////////////////////////////////////////////////////////////////

ezResult ezActionMapManager::RegisterActionMap(const ezHashedString& sMapping)
{
  auto it = s_Mappings.Find(sMapping);
  if (it.IsValid())
    return EZ_FAILURE;

  s_Mappings.Insert(sMapping, EZ_DEFAULT_NEW(ezActionMap));
  return EZ_SUCCESS;
}

ezResult ezActionMapManager::UnregisterActionMap(const ezHashedString& sMapping)
{
  auto it = s_Mappings.Find(sMapping);
  if (!it.IsValid())
    return EZ_FAILURE;

  EZ_DEFAULT_DELETE(it.Value());
  s_Mappings.Remove(it);
  return EZ_SUCCESS;
}

ezActionMap* ezActionMapManager::GetActionMap(const ezHashedString& sMapping)
{
  auto it = s_Mappings.Find(sMapping);
  if (!it.IsValid())
    return nullptr;

  return it.Value();
}


////////////////////////////////////////////////////////////////////////
// ezActionMapManager private functions
////////////////////////////////////////////////////////////////////////

void ezActionMapManager::Startup()
{
}

void ezActionMapManager::Shutdown()
{
  for (auto it = s_Mappings.GetIterator(); it.IsValid();)
  {
    auto oldIt = it;
    ezResult res = UnregisterActionMap(oldIt.Key());
    EZ_ASSERT_DEV(res == EZ_SUCCESS, "Failed to call UnregisterActionMap successfully!");
    ++it;
  }
  EZ_ASSERT_DEV(s_Mappings.IsEmpty(), "ezActionMapManager::Shutdown did not remove all action maps!");
  s_Mappings.Clear();
}
