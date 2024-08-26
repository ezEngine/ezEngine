#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClassResource.h>
#include <Core/Scripting/ScriptWorldModule.h>

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezScriptWorldModule);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScriptWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezScriptWorldModule::ezScriptWorldModule(ezWorld* pWorld)
  : ezWorldModule(pWorld)
{
}

ezScriptWorldModule::~ezScriptWorldModule() = default;

void ezScriptWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto updateDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezScriptWorldModule::CallUpdateFunctions, this);
    updateDesc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PreAsync;

    RegisterUpdateFunction(updateDesc);
  }
}

void ezScriptWorldModule::WorldClear()
{
  m_Scheduler.Clear();
}

void ezScriptWorldModule::AddUpdateFunctionToSchedule(const ezAbstractFunctionProperty* pFunction, void* pInstance, ezTime updateInterval, bool bOnlyWhenSimulating)
{
  FunctionContext context;
  context.m_pFunctionAndFlags.SetPtrAndFlags(pFunction, bOnlyWhenSimulating ? FunctionContext::Flags::OnlyWhenSimulating : FunctionContext::Flags::None);
  context.m_pInstance = pInstance;

  m_Scheduler.AddOrUpdateWork(context, updateInterval);
}

void ezScriptWorldModule::RemoveUpdateFunctionToSchedule(const ezAbstractFunctionProperty* pFunction, void* pInstance)
{
  FunctionContext context;
  context.m_pFunctionAndFlags.SetPtr(pFunction);
  context.m_pInstance = pInstance;

  m_Scheduler.RemoveWork(context);
}

ezScriptCoroutineHandle ezScriptWorldModule::CreateCoroutine(const ezRTTI* pCoroutineType, ezStringView sName, ezScriptInstance& inout_instance, ezScriptCoroutineCreationMode::Enum creationMode, ezScriptCoroutine*& out_pCoroutine)
{
  if (creationMode != ezScriptCoroutineCreationMode::AllowOverlap)
  {
    ezScriptCoroutine* pOverlappingCoroutine = nullptr;

    auto& runningCoroutines = m_InstanceToScriptCoroutines[&inout_instance];
    for (auto& hCoroutine : runningCoroutines)
    {
      ezUniquePtr<ezScriptCoroutine>* pCoroutine = nullptr;
      if (m_RunningScriptCoroutines.TryGetValue(hCoroutine.GetInternalID(), pCoroutine) && (*pCoroutine)->GetName() == sName)
      {
        pOverlappingCoroutine = pCoroutine->Borrow();
        break;
      }
    }

    if (pOverlappingCoroutine != nullptr)
    {
      if (creationMode == ezScriptCoroutineCreationMode::StopOther)
      {
        StopAndDeleteCoroutine(pOverlappingCoroutine->GetHandle());
      }
      else if (creationMode == ezScriptCoroutineCreationMode::DontCreateNew)
      {
        out_pCoroutine = nullptr;
        return ezScriptCoroutineHandle();
      }
      else
      {
        EZ_ASSERT_NOT_IMPLEMENTED;
      }
    }
  }

  auto pCoroutine = pCoroutineType->GetAllocator()->Allocate<ezScriptCoroutine>(ezScriptAllocator::GetAllocator());

  ezScriptCoroutineId id = m_RunningScriptCoroutines.Insert(pCoroutine);
  pCoroutine->Initialize(id, sName, inout_instance, *this);

  m_InstanceToScriptCoroutines[&inout_instance].PushBack(ezScriptCoroutineHandle(id));

  out_pCoroutine = pCoroutine;
  return ezScriptCoroutineHandle(id);
}

void ezScriptWorldModule::StartCoroutine(ezScriptCoroutineHandle hCoroutine, ezArrayPtr<ezVariant> arguments)
{
  ezUniquePtr<ezScriptCoroutine>* pCoroutine = nullptr;
  if (m_RunningScriptCoroutines.TryGetValue(hCoroutine.GetInternalID(), pCoroutine))
  {
    (*pCoroutine)->StartWithVarargs(arguments);
    (*pCoroutine)->UpdateAndSchedule();
  }
}

void ezScriptWorldModule::StopAndDeleteCoroutine(ezScriptCoroutineHandle hCoroutine)
{
  ezUniquePtr<ezScriptCoroutine> pCoroutine;
  if (m_RunningScriptCoroutines.Remove(hCoroutine.GetInternalID(), &pCoroutine) == false)
    return;

  pCoroutine->Stop();
  pCoroutine->Deinitialize();
  m_DeadScriptCoroutines.PushBack(std::move(pCoroutine));
}

void ezScriptWorldModule::StopAndDeleteCoroutine(ezStringView sName, ezScriptInstance* pInstance)
{
  if (auto pCoroutines = m_InstanceToScriptCoroutines.GetValue(pInstance))
  {
    for (ezUInt32 i = 0; i < pCoroutines->GetCount();)
    {
      auto hCoroutine = (*pCoroutines)[i];

      ezUniquePtr<ezScriptCoroutine>* pCoroutine = nullptr;
      if (m_RunningScriptCoroutines.TryGetValue(hCoroutine.GetInternalID(), pCoroutine) && (*pCoroutine)->GetName() == sName)
      {
        StopAndDeleteCoroutine(hCoroutine);
      }
      else
      {
        ++i;
      }
    }
  }
}

void ezScriptWorldModule::StopAndDeleteAllCoroutines(ezScriptInstance* pInstance)
{
  if (auto pCoroutines = m_InstanceToScriptCoroutines.GetValue(pInstance))
  {
    for (auto hCoroutine : *pCoroutines)
    {
      StopAndDeleteCoroutine(hCoroutine);
    }
  }
}

bool ezScriptWorldModule::IsCoroutineFinished(ezScriptCoroutineHandle hCoroutine) const
{
  return m_RunningScriptCoroutines.Contains(hCoroutine.GetInternalID()) == false;
}

void ezScriptWorldModule::CallUpdateFunctions(const ezWorldModule::UpdateContext& context)
{
  EZ_IGNORE_UNUSED(context);

  ezWorld* pWorld = GetWorld();

  ezTime deltaTime;
  if (pWorld->GetWorldSimulationEnabled())
  {
    deltaTime = pWorld->GetClock().GetTimeDiff();
  }
  else
  {
    deltaTime = ezClock::GetGlobalClock()->GetTimeDiff();
  }

  m_Scheduler.Update(deltaTime,
    [this](const FunctionContext& context, ezTime deltaTime)
    {
      if (GetWorld()->GetWorldSimulationEnabled() || context.m_pFunctionAndFlags.GetFlags() == FunctionContext::Flags::None)
      {
        ezVariant args[] = {deltaTime};
        ezVariant returnValue;
        context.m_pFunctionAndFlags->Execute(context.m_pInstance, ezMakeArrayPtr(args), returnValue);
      }
    });

  // Delete dead coroutines
  for (ezUInt32 i = 0; i < m_DeadScriptCoroutines.GetCount(); ++i)
  {
    auto& pCoroutine = m_DeadScriptCoroutines[i];
    ezScriptInstance* pInstance = pCoroutine->GetScriptInstance();
    auto pCoroutines = m_InstanceToScriptCoroutines.GetValue(pInstance);
    EZ_ASSERT_DEV(pCoroutines != nullptr, "Implementation error");

    pCoroutines->RemoveAndSwap(pCoroutine->GetHandle());
    if (pCoroutines->IsEmpty())
    {
      m_InstanceToScriptCoroutines.Remove(pInstance);
    }

    pCoroutine = nullptr;
  }
  m_DeadScriptCoroutines.Clear();
}


EZ_STATICLINK_FILE(Core, Core_Scripting_Implementation_ScriptWorldModule);
