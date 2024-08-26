#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <VisualScriptPlugin/Runtime/VisualScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

ezVisualScriptCoroutine::ezVisualScriptCoroutine(const ezSharedPtr<const ezVisualScriptGraphDescription>& pDesc)
  : m_LocalDataStorage(pDesc->GetLocalDataDesc())
  , m_Context(pDesc)
{
}

ezVisualScriptCoroutine::~ezVisualScriptCoroutine() = default;

void ezVisualScriptCoroutine::StartWithVarargs(ezArrayPtr<ezVariant> arguments)
{
  m_LocalDataStorage.AllocateStorage();

  auto pVisualScriptInstance = static_cast<ezVisualScriptInstance*>(GetScriptInstance());
  m_Context.Initialize(*pVisualScriptInstance, m_LocalDataStorage, arguments);
}

void ezVisualScriptCoroutine::Stop()
{
  m_Context.Deinitialize();
}

ezScriptCoroutine::Result ezVisualScriptCoroutine::Update(ezTime deltaTimeSinceLastUpdate)
{
  auto result = m_Context.Execute(deltaTimeSinceLastUpdate);
  if (result.m_NextExecAndState == ezVisualScriptExecutionContext::ExecResult::State::ContinueLater)
  {
    return Result::Running(result.m_MaxDelay);
  }

  return Result::Completed();
}

//////////////////////////////////////////////////////////////////////////

ezVisualScriptCoroutineAllocator::ezVisualScriptCoroutineAllocator(const ezSharedPtr<const ezVisualScriptGraphDescription>& pDesc)
  : m_pDesc(pDesc)
{
}

void ezVisualScriptCoroutineAllocator::Deallocate(void* pObject, ezAllocator* pAllocator /*= nullptr*/)
{
  EZ_REPORT_FAILURE("Deallocate is not supported");
}

ezInternal::NewInstance<void> ezVisualScriptCoroutineAllocator::AllocateInternal(ezAllocator* pAllocator)
{
  return EZ_SCRIPT_NEW(ezVisualScriptCoroutine, m_pDesc);
}
