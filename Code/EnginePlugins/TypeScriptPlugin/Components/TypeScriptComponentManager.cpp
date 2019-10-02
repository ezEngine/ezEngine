#include <TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>

ezTypeScriptComponentManager::ezTypeScriptComponentManager(ezWorld* pWorld)
  : SUPER(pWorld)
{
}

ezTypeScriptComponentManager::~ezTypeScriptComponentManager() = default;

void ezTypeScriptComponentManager::Initialize()
{
  SUPER::Initialize();

  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezTypeScriptComponentManager::Update, this);
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_Phase = UpdateFunctionDesc::Phase::PreAsync;

  RegisterUpdateFunction(desc);

  ezFileSystem::AddDataDirectory(">sdk/Data/Tools/ezEditor/TypeScript", "TypeScript", "TypeScript");

  m_Transpiler.StartLoadTranspiler();
}

void ezTypeScriptComponentManager::Deinitialize()
{
  ezFileSystem::RemoveDataDirectoryGroup("TypeScript");

  SUPER::Deinitialize();
}

void ezTypeScriptComponentManager::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  m_TsBinding.Initialize(m_Transpiler, *GetWorld());
}

void ezTypeScriptComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndSimulating())
    {
      it->Update(m_TsBinding);
    }
  }
}
