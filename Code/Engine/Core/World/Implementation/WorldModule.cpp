#include <Core/PCH.h>
#include <Core/World/WorldModule.h>

ezDynamicArray<ezWorldModule*> ezWorldModule::s_AllWorldModules;

ezWorldModule::ezWorldModule()
{
  s_AllWorldModules.PushBack(this);
}

ezWorldModule::~ezWorldModule()
{
  s_AllWorldModules.RemoveSwap(this);
}

void ezWorldModule::Startup(ezWorld* pOwner)
{
  m_pOwnerWorld = pOwner;
  InternalStartup();
}

void ezWorldModule::BeforeWorldDestruction()
{
  InternalBeforeWorldDestruction();
}

void ezWorldModule::AfterWorldDestruction()
{
  InternalAfterWorldDestruction();
}

void ezWorldModule::Update()
{
  InternalUpdate();
}

void ezWorldModule::Reinit()
{
  InternalReinit();
}

ezWorldModule* ezWorldModule::FindModule(const ezWorld* pWorld, const ezRTTI* pWorldModuleType)
{
  EZ_ASSERT_DEV(pWorld != nullptr, "No module will be registered for the nullptr world");

  for (auto pModule : s_AllWorldModules)
  {
    if (pModule->GetWorld() != pWorld)
      continue;

    if (pModule->GetDynamicRTTI()->IsDerivedFrom(pWorldModuleType))
      return pModule;
  }

  return nullptr;
}
