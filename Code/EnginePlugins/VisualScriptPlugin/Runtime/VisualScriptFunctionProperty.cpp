#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Core/Scripting/ScriptWorldModule.h>
#include <VisualScriptPlugin/Runtime/VisualScriptFunctionProperty.h>

ezVisualScriptFunctionProperty::ezVisualScriptFunctionProperty(ezStringView sName, const ezSharedPtr<const ezVisualScriptGraphDescription>& pDesc)
  : ezScriptFunctionProperty(sName)
  , m_pDesc(pDesc)
  , m_LocalDataStorage(pDesc->GetLocalDataDesc())
{
  EZ_ASSERT_DEBUG(m_pDesc->IsCoroutine() == false, "Must not be a coroutine");

  m_LocalDataStorage.AllocateStorage();
}

ezVisualScriptFunctionProperty::~ezVisualScriptFunctionProperty() = default;

void ezVisualScriptFunctionProperty::Execute(void* pInstance, ezArrayPtr<ezVariant> arguments, ezVariant& out_returnValue) const
{
  EZ_ASSERT_DEBUG(pInstance != nullptr, "Invalid instance");
  auto pVisualScriptInstance = static_cast<ezVisualScriptInstance*>(pInstance);

  ezVisualScriptExecutionContext context(m_pDesc);
  context.Initialize(*pVisualScriptInstance, m_LocalDataStorage, arguments);

  auto result = context.Execute(ezTime::MakeZero());
  EZ_ASSERT_DEBUG(result.m_NextExecAndState != ezVisualScriptExecutionContext::ExecResult::State::ContinueLater, "A non-coroutine function must not return 'ContinueLater'");

  // TODO: return value
}
