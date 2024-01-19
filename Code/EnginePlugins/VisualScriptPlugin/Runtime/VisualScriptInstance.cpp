#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

ezVisualScriptInstance::ezVisualScriptInstance(ezReflectedClass& inout_owner, ezWorld* pWorld, const ezSharedPtr<ezVisualScriptDataStorage>& pConstantDataStorage, const ezSharedPtr<const ezVisualScriptDataDescription>& pInstanceDataDesc, const ezSharedPtr<ezVisualScriptInstanceDataMapping>& pInstanceDataMapping)
  : ezScriptInstance(inout_owner, pWorld)
  , m_pConstantDataStorage(pConstantDataStorage)
  , m_pInstanceDataMapping(pInstanceDataMapping)
{
  if (pInstanceDataDesc != nullptr)
  {
    m_pInstanceDataStorage = EZ_SCRIPT_NEW(ezVisualScriptDataStorage, pInstanceDataDesc);
    m_pInstanceDataStorage->AllocateStorage();

    for (auto& it : m_pInstanceDataMapping->m_Content)
    {
      auto& instanceData = it.Value();
      m_pInstanceDataStorage->SetDataFromVariant(instanceData.m_DataOffset, instanceData.m_DefaultValue, 0);
    }
  }
}

void ezVisualScriptInstance::SetInstanceVariable(const ezHashedString& sName, const ezVariant& value)
{
  if (m_pInstanceDataMapping == nullptr)
    return;

  ezVisualScriptInstanceData* pInstanceData = nullptr;
  if (m_pInstanceDataMapping->m_Content.TryGetValue(sName, pInstanceData) == false)
    return;

  ezResult conversionStatus = EZ_FAILURE;
  ezVariantType::Enum targetType = ezVisualScriptDataType::GetVariantType(pInstanceData->m_DataOffset.GetType());

  ezVariant convertedValue = value.ConvertTo(targetType, &conversionStatus);
  if (conversionStatus.Failed())
  {
    ezLog::Error("Can't apply instance variable '{}' because the given value of type '{}' can't be converted the expected target type '{}'", sName, value.GetType(), targetType);
    return;
  }

  m_pInstanceDataStorage->SetDataFromVariant(pInstanceData->m_DataOffset, convertedValue, 0);
}

ezVariant ezVisualScriptInstance::GetInstanceVariable(const ezHashedString& sName)
{
  if (m_pInstanceDataMapping == nullptr)
    return ezVariant();

  ezVisualScriptInstanceData* pInstanceData = nullptr;
  if (m_pInstanceDataMapping->m_Content.TryGetValue(sName, pInstanceData) == false)
    return ezVariant();

  return m_pInstanceDataStorage->GetDataAsVariant(pInstanceData->m_DataOffset, nullptr, 0);
}
