#include <Core/CorePCH.h>

#include <Core/World/World.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezWorldModule::ezWorldModule(ezWorld* pWorld)
  : m_pWorld(pWorld)
{
}

ezWorldModule::~ezWorldModule() = default;

ezUInt32 ezWorldModule::GetWorldIndex() const
{
  return GetWorld()->GetIndex();
}

// protected methods

void ezWorldModule::RegisterUpdateFunction(const UpdateFunctionDesc& desc)
{
  m_pWorld->RegisterUpdateFunction(desc);
}

void ezWorldModule::DeregisterUpdateFunction(const UpdateFunctionDesc& desc)
{
  m_pWorld->DeregisterUpdateFunction(desc);
}

ezAllocator* ezWorldModule::GetAllocator()
{
  return m_pWorld->GetAllocator();
}

ezInternal::WorldLargeBlockAllocator* ezWorldModule::GetBlockAllocator()
{
  return m_pWorld->GetBlockAllocator();
}

bool ezWorldModule::GetWorldSimulationEnabled() const
{
  return m_pWorld->GetWorldSimulationEnabled();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Core, WorldModuleFactory)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Reflection"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezPlugin::Events().AddEventHandler(ezWorldModuleFactory::PluginEventHandler);
    ezWorldModuleFactory::GetInstance()->FillBaseTypeIds();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezPlugin::Events().RemoveEventHandler(ezWorldModuleFactory::PluginEventHandler);
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

static ezWorldModuleTypeId s_uiNextTypeId = 0;
static ezDynamicArray<ezWorldModuleTypeId> s_freeTypeIds;
static constexpr ezWorldModuleTypeId s_InvalidWorldModuleTypeId = ezWorldModuleTypeId(-1);

ezWorldModuleFactory::ezWorldModuleFactory() = default;

// static
ezWorldModuleFactory* ezWorldModuleFactory::GetInstance()
{
  static ezWorldModuleFactory* pInstance = new ezWorldModuleFactory();
  return pInstance;
}

ezWorldModuleTypeId ezWorldModuleFactory::GetTypeId(const ezRTTI* pRtti)
{
  ezWorldModuleTypeId uiTypeId = s_InvalidWorldModuleTypeId;
  m_TypeToId.TryGetValue(pRtti, uiTypeId);
  return uiTypeId;
}

ezWorldModule* ezWorldModuleFactory::CreateWorldModule(ezWorldModuleTypeId typeId, ezWorld* pWorld)
{
  if (typeId < m_CreatorFuncs.GetCount())
  {
    CreatorFunc func = m_CreatorFuncs[typeId].m_Func;
    return (*func)(pWorld->GetAllocator(), pWorld);
  }

  return nullptr;
}

void ezWorldModuleFactory::RegisterInterfaceImplementation(ezStringView sInterfaceName, ezStringView sImplementationName)
{
  m_InterfaceImplementations.Insert(sInterfaceName, sImplementationName);

  ezStringBuilder sTemp = sInterfaceName;
  const ezRTTI* pInterfaceRtti = ezRTTI::FindTypeByName(sTemp);

  sTemp = sImplementationName;
  const ezRTTI* pImplementationRtti = ezRTTI::FindTypeByName(sTemp);

  if (pInterfaceRtti != nullptr && pImplementationRtti != nullptr)
  {
    m_TypeToId[pInterfaceRtti] = m_TypeToId[pImplementationRtti];
    return;
  }

  // Clear existing mapping if it maps to the wrong type
  ezUInt16 uiTypeId;
  if (pInterfaceRtti != nullptr && m_TypeToId.TryGetValue(pInterfaceRtti, uiTypeId))
  {
    if (m_CreatorFuncs[uiTypeId].m_pRtti->GetTypeName() != sImplementationName)
    {
      EZ_ASSERT_DEV(pImplementationRtti == nullptr, "Implementation error");
      m_TypeToId.Remove(pInterfaceRtti);
    }
  }
}
ezWorldModuleTypeId ezWorldModuleFactory::RegisterWorldModule(const ezRTTI* pRtti, CreatorFunc creatorFunc)
{
  EZ_ASSERT_DEV(pRtti != ezGetStaticRTTI<ezWorldModule>(), "Trying to register a world module that is not reflected!");
  EZ_ASSERT_DEV(
    m_TypeToId.GetCount() < ezWorld::GetMaxNumWorldModules(), "Max number of world modules reached: {}", ezWorld::GetMaxNumWorldModules());

  ezWorldModuleTypeId uiTypeId = s_InvalidWorldModuleTypeId;
  if (m_TypeToId.TryGetValue(pRtti, uiTypeId))
  {
    return uiTypeId;
  }

  if (s_freeTypeIds.IsEmpty())
  {
    EZ_ASSERT_DEV(s_uiNextTypeId < EZ_MAX_WORLD_MODULE_TYPES - 1, "World module id overflow!");

    uiTypeId = s_uiNextTypeId++;
  }
  else
  {
    uiTypeId = s_freeTypeIds.PeekBack();
    s_freeTypeIds.PopBack();
  }

  m_TypeToId.Insert(pRtti, uiTypeId);

  m_CreatorFuncs.EnsureCount(uiTypeId + 1);

  auto& creatorFuncContext = m_CreatorFuncs[uiTypeId];
  creatorFuncContext.m_Func = creatorFunc;
  creatorFuncContext.m_pRtti = pRtti;

  return uiTypeId;
}

// static
void ezWorldModuleFactory::PluginEventHandler(const ezPluginEvent& EventData)
{
  if (EventData.m_EventType == ezPluginEvent::AfterLoadingBeforeInit)
  {
    ezWorldModuleFactory::GetInstance()->FillBaseTypeIds();
  }

  if (EventData.m_EventType == ezPluginEvent::AfterUnloading)
  {
    ezWorldModuleFactory::GetInstance()->ClearUnloadedTypeToIDs();
  }
}

namespace
{
  struct NewEntry
  {
    EZ_DECLARE_POD_TYPE();

    const ezRTTI* m_pRtti;
    ezWorldModuleTypeId m_uiTypeId;
  };
} // namespace

void ezWorldModuleFactory::AdjustBaseTypeId(const ezRTTI* pParentRtti, const ezRTTI* pRtti, ezUInt16 uiParentTypeId)
{
  ezDynamicArray<ezPlugin::PluginInfo> infos;
  ezPlugin::GetAllPluginInfos(infos);

  auto HasManualDependency = [&](ezStringView sPluginName) -> bool
  {
    for (const auto& p : infos)
    {
      if (p.m_sName == sPluginName)
      {
        return !p.m_LoadFlags.IsSet(ezPluginLoadFlags::CustomDependency);
      }
    }

    return false;
  };

  ezStringView szPlugin1 = m_CreatorFuncs[uiParentTypeId].m_pRtti->GetPluginName();
  ezStringView szPlugin2 = pRtti->GetPluginName();

  const bool bPrio1 = HasManualDependency(szPlugin1);
  const bool bPrio2 = HasManualDependency(szPlugin2);

  if (bPrio1 && !bPrio2)
  {
    // keep the previous one
    return;
  }

  if (!bPrio1 && bPrio2)
  {
    // take the new one
    m_TypeToId[pParentRtti] = m_TypeToId[pRtti];
    return;
  }

  ezLog::Error("Interface '{}' is already implemented by '{}'. Specify which implementation should be used via RegisterInterfaceImplementation() or WorldModules.ddl config file.", pParentRtti->GetTypeName(), m_CreatorFuncs[uiParentTypeId].m_pRtti->GetTypeName());
}

void ezWorldModuleFactory::FillBaseTypeIds()
{
  // m_TypeToId contains RTTI types for ezWorldModules and ezComponents
  // m_TypeToId[ezComponent] maps to TypeID for its respective ezComponentManager
  // m_TypeToId[ezWorldModule] maps to TypeID for itself OR in case of an interface to the derived type that implements the interface
  // after types are registered we only have a mapping for m_TypeToId[ezWorldModule(impl)] and now we want to add
  // the mapping for m_TypeToId[ezWorldModule(interface)], such that querying the TypeID for the interface works as well
  // and yields the implementation

  ezHybridArray<NewEntry, 64, ezStaticsAllocatorWrapper> newEntries;
  const ezRTTI* pModuleRtti = ezGetStaticRTTI<ezWorldModule>(); // base type where we want to stop iterating upwards

  // explicit mappings
  for (auto it = m_InterfaceImplementations.GetIterator(); it.IsValid(); ++it)
  {
    const ezRTTI* pInterfaceRtti = ezRTTI::FindTypeByName(it.Key());
    const ezRTTI* pImplementationRtti = ezRTTI::FindTypeByName(it.Value());

    if (pInterfaceRtti != nullptr && pImplementationRtti != nullptr)
    {
      m_TypeToId[pInterfaceRtti] = m_TypeToId[pImplementationRtti];
    }
  }

  // automatic mappings
  for (auto it = m_TypeToId.GetIterator(); it.IsValid(); ++it)
  {
    const ezRTTI* pRtti = it.Key();

    // ignore components, we only want to fill out mappings for the base types of world modules
    if (!pRtti->IsDerivedFrom<ezWorldModule>())
      continue;

    const ezWorldModuleTypeId uiTypeId = it.Value();

    for (const ezRTTI* pParentRtti = pRtti->GetParentType(); pParentRtti != pModuleRtti; pParentRtti = pParentRtti->GetParentType())
    {
      // we are only interested in parent types that are pure interfaces
      if (!pParentRtti->GetTypeFlags().IsSet(ezTypeFlags::Abstract))
        continue;

      // skip if we have an explicit mapping for this interface, they are already handled above
      if (m_InterfaceImplementations.GetValue(pParentRtti->GetTypeName()) != nullptr)
        continue;


      if (ezUInt16* pParentTypeId = m_TypeToId.GetValue(pParentRtti))
      {
        if (*pParentTypeId != uiTypeId)
        {
          AdjustBaseTypeId(pParentRtti, pRtti, *pParentTypeId);
        }
      }
      else
      {
        auto& newEntry = newEntries.ExpandAndGetRef();
        newEntry.m_pRtti = pParentRtti;
        newEntry.m_uiTypeId = uiTypeId;
      }
    }
  }

  // delayed insertion to not interfere with the iteration above
  for (auto& newEntry : newEntries)
  {
    m_TypeToId.Insert(newEntry.m_pRtti, newEntry.m_uiTypeId);
  }
}

void ezWorldModuleFactory::ClearUnloadedTypeToIDs()
{
  ezSet<const ezRTTI*> allRttis;
  ezRTTI::ForEachType([&](const ezRTTI* pRtti)
    { allRttis.Insert(pRtti); });

  ezSet<ezWorldModuleTypeId> mappedIdsToRemove;

  for (auto it = m_TypeToId.GetIterator(); it.IsValid();)
  {
    const ezRTTI* pRtti = it.Key();
    const ezWorldModuleTypeId uiTypeId = it.Value();

    if (!allRttis.Contains(pRtti))
    {
      // type got removed, clear it from the map
      it = m_TypeToId.Remove(it);

      // and record that all other types that map to the same typeId also must be removed
      mappedIdsToRemove.Insert(uiTypeId);
    }
    else
    {
      ++it;
    }
  }

  // now remove all mappings that map to an invalid typeId
  // this can be more than one, since we can map multiple (interface) types to the same implementation
  for (auto it = m_TypeToId.GetIterator(); it.IsValid();)
  {
    const ezWorldModuleTypeId uiTypeId = it.Value();

    if (mappedIdsToRemove.Contains(uiTypeId))
    {
      it = m_TypeToId.Remove(it);
    }
    else
    {
      ++it;
    }
  }

  // Finally, adding all invalid typeIds to the free list for reusing later
  for (ezWorldModuleTypeId removedId : mappedIdsToRemove)
  {
    s_freeTypeIds.PushBack(removedId);
  }
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_WorldModule);
