#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiPerception.h>
#include <AiPlugin/UtilityAI/Framework/AiScript.h>
#include <Core/Scripting/ScriptWorldModule.h>
#include <Core/World/World.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAiScriptContext, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(GetOwner),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetNow),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetPerceptionCount),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetPerception, In, "Index"),
    EZ_SCRIPT_FUNCTION_PROPERTY(SetValue, In, "Value"),
    EZ_SCRIPT_FUNCTION_PROPERTY(SetCanRun, In, "CanRun"),
    EZ_SCRIPT_FUNCTION_PROPERTY(Restart),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezAiScriptContext::ezAiScriptContext(const ezAiContext& ctx)
  : m_pWorld(ctx.m_Owner.GetWorld())
  , m_hOwner(ctx.m_Owner.GetHandle())
{
}

ezGameObject* ezAiScriptContext::GetOwner() const
{
  ezGameObject* pOwner = nullptr;
  if (m_pWorld == nullptr || !m_pWorld->TryGetObject(m_hOwner, pOwner))
    return nullptr;

  return pOwner;
}

ezTime ezAiScriptContext::GetNow() const
{
  // the same as ezAiContext::m_Now
  return m_pWorld ? m_pWorld->GetClock().GetAccumulatedTime() : ezTime::MakeZero();
}

ezUInt32 ezAiScriptContext::GetPerceptionCount() const
{
  return m_Perceptions.GetCount();
}

ezAiPerception* ezAiScriptContext::GetPerception(ezUInt32 uiIndex) const
{
  if (uiIndex >= m_Perceptions.GetCount())
    return nullptr;

  return const_cast<ezAiPerception*>(m_Perceptions[uiIndex]);
}

void ezAiScriptContext::SetValue(float fValue)
{
  m_fValue = fValue;
}

void ezAiScriptContext::SetCanRun(bool bCanRun)
{
  m_bCanRun = bCanRun;
}

void ezAiScriptContext::Restart()
{
  m_bRestart = true;
}

//////////////////////////////////////////////////////////////////////////

ezAiScriptInstance::ezAiScriptInstance() = default;

ezAiScriptInstance::~ezAiScriptInstance()
{
  Clear();
}

const ezAbstractFunctionProperty* ezAiScriptInstance::GetFunction(const ezAiContext& ctx, ezUInt32 uiFunctionIndex, ezReflectedClass& ref_owner, const ezRTTI* pBaseType, ezStringView sScriptClass, const ezArrayMap<ezHashedString, ezVariant>& parameters)
{
  if (m_pInstance == nullptr && !m_bFailed)
  {
    m_pWorld = ctx.m_Owner.GetWorld();
    m_pOwner = &ref_owner;
    m_pBaseType = pBaseType;
    m_pParameters = &parameters;

    if (sScriptClass.IsEmpty())
    {
      ezLog::Error(ctx.m_pLog, "{}: no script class set.", pBaseType->GetTypeName());
      m_bFailed = true;
      return nullptr;
    }

    m_hScriptClass = ezResourceManager::LoadResource<ezScriptClassResource>(sScriptClass);
    Instantiate();

    m_pWorld->AddResourceReloadFunction(m_hScriptClass, ezComponentHandle(), this,
      [](ezWorld::ResourceReloadContext& context)
      {
        auto pThis = static_cast<ezAiScriptInstance*>(context.m_pUserData);
        pThis->m_bFailed = false;
        pThis->Instantiate();
      });
  }

  if (m_pScriptType == nullptr || m_pInstance == nullptr)
    return nullptr;

  return m_pScriptType->GetFunctionByIndex(uiFunctionIndex);
}

void ezAiScriptInstance::Call(const ezAiContext& ctx, const ezAbstractFunctionProperty* pFunction, ezArrayPtr<ezVariant> arguments)
{
  ezVariant returnValue;
  pFunction->Execute(m_pInstance.Borrow(), arguments, returnValue);

  if (returnValue.IsA<ezScriptCoroutineHandle>())
  {
    // the first step already ran during the call, the coroutine is only still alive if it waits
    ezScriptWorldModule* pModule = m_pWorld->GetOrCreateModule<ezScriptWorldModule>();
    const ezScriptCoroutineHandle hCoroutine = returnValue.Get<ezScriptCoroutineHandle>();

    if (!pModule->IsCoroutineFinished(hCoroutine))
    {
      if (!m_bWarnedAboutCoroutine)
      {
        ezLog::Warning(ctx.m_pLog, "Script '{}': '{}' can't wait, it was stopped. Queue actions instead.", m_hScriptClass.GetResourceID(), pFunction->GetPropertyName());
        m_bWarnedAboutCoroutine = true;
      }

      pModule->StopAndDeleteCoroutine(hCoroutine);
    }
  }
}

void ezAiScriptInstance::Instantiate()
{
  if (m_pInstance != nullptr)
  {
    m_pWorld->GetOrCreateModule<ezScriptWorldModule>()->StopAndDeleteAllCoroutines(m_pInstance.Borrow());
  }

  m_pInstance = nullptr;
  m_pScriptType = nullptr;

  ezResourceLock<ezScriptClassResource> pScript(m_hScriptClass, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pScript.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    ezLog::Error("Failed to load script '{}'", m_hScriptClass.GetResourceID());
    m_bFailed = true;
    return;
  }

  auto pScriptType = pScript->GetType();
  if (pScriptType == nullptr || !pScriptType->IsDerivedFrom(m_pBaseType))
  {
    ezLog::Error("Script '{}' is not derived from '{}'", m_hScriptClass.GetResourceID(), m_pBaseType->GetTypeName());
    m_bFailed = true;
    return;
  }

  m_pScriptType = pScriptType;
  m_pInstance = pScript->Instantiate(*m_pOwner, m_pWorld);

  if (m_pInstance != nullptr)
  {
    m_pInstance->SetInstanceVariables(*m_pParameters);
  }
}

void ezAiScriptInstance::Clear()
{
  if (m_pWorld == nullptr)
    return;

  if (m_pInstance != nullptr)
  {
    m_pWorld->GetOrCreateModule<ezScriptWorldModule>()->StopAndDeleteAllCoroutines(m_pInstance.Borrow());
  }

  if (m_hScriptClass.IsValid())
  {
    m_pWorld->RemoveResourceReloadFunction(m_hScriptClass, ezComponentHandle(), this);
  }

  m_pInstance = nullptr;
  m_pScriptType = nullptr;
  m_pWorld = nullptr;
}
