#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

ezVisualScriptInstance::ezVisualScriptInstance(ezReflectedClass& ref_owner, ezWorld* pWorld, const ezSharedPtr<const ezVisualScriptDataStorage>& pConstantDataStorage, const ezSharedPtr<const ezVisualScriptDataDescription>& pVariableDataDesc)
  : m_Owner(ref_owner)
  , m_pWorld(pWorld)
  , m_pConstantDataStorage(pConstantDataStorage)
{
  if (pVariableDataDesc != nullptr)
  {
    m_pVariableDataStorage = EZ_DEFAULT_NEW(ezVisualScriptDataStorage, pVariableDataDesc);
    m_pVariableDataStorage->AllocateStorage();
  }
}

void ezVisualScriptInstance::ApplyParameters(const ezArrayMap<ezHashedString, ezVariant>& parameters)
{
}

//////////////////////////////////////////////////////////////////////////

ezVisualScriptExecutionContext::ezVisualScriptExecutionContext(ezUniquePtr<ezVisualScriptGraphDescription>&& pDesc)
  : m_pDesc(std::move(pDesc))
{
}

ezResult ezVisualScriptExecutionContext::Initialize(ezVisualScriptInstance& ref_instance, ezArrayPtr<ezVariant> arguments)
{
  m_pInstance = &ref_instance;

  auto pNode = m_pDesc->GetNode(0);
  EZ_ASSERT_DEV(pNode->m_Type == ezVisualScriptNodeDescription::Type::EntryCall, "Invalid entry node");

  for (ezUInt32 i = 0; i < arguments.GetCount(); ++i)
  {
    m_pInstance->SetDataFromVariant(pNode->GetOutputDataOffset(i), arguments[i]);
  }

  m_uiCurrentNode = pNode->GetExecutionIndex(0);
  return EZ_SUCCESS;
}

ezVisualScriptExecutionContext::ReturnValue::Enum ezVisualScriptExecutionContext::Execute()
{
  EZ_ASSERT_DEV(m_pInstance != nullptr, "Invalid instance");
  ++m_pInstance->m_uiExecutionCounter;

  auto pNode = m_pDesc->GetNode(m_uiCurrentNode);
  while (pNode != nullptr)
  {
    int result = pNode->m_Function(*m_pInstance, *pNode);
    if (result < ReturnValue::Completed)
    {
      return static_cast<ReturnValue::Enum>(result);
    }

    m_uiCurrentNode = pNode->GetExecutionIndex(result);
    pNode = m_pDesc->GetNode(m_uiCurrentNode);
  }

  return ReturnValue::Completed;
}

//////////////////////////////////////////////////////////////////////////

ezVisualScriptFunctionProperty::ezVisualScriptFunctionProperty(const char* szPropertyName, ezUniquePtr<ezVisualScriptGraphDescription>&& pDesc)
  : ezAbstractFunctionProperty(nullptr)
  , m_ExecutionContext(std::move(pDesc))
{
  m_sPropertyNameStorage.Assign(szPropertyName);
  m_szPropertyName = m_sPropertyNameStorage.GetData();
}

ezVisualScriptFunctionProperty::~ezVisualScriptFunctionProperty() = default;

void ezVisualScriptFunctionProperty::Execute(void* pInstance, ezArrayPtr<ezVariant> arguments, ezVariant& out_returnValue) const
{
  EZ_ASSERT_DEBUG(pInstance != nullptr, "Invalid instance");
  auto pVisualScriptInstance = static_cast<ezVisualScriptInstance*>(pInstance);

  if (m_ExecutionContext.Initialize(*pVisualScriptInstance, arguments).Failed())
  {
    return;
  }

  m_ExecutionContext.Execute();
}
