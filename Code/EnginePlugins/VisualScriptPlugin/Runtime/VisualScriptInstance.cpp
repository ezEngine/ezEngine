#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

ezVisualScriptInstance::ezVisualScriptInstance(ezReflectedClass& inout_owner, ezWorld* pWorld, const ezSharedPtr<ezVisualScriptDataStorage>& pConstantDataStorage, const ezSharedPtr<const ezVisualScriptDataDescription>& pInstanceDataDesc, const ezSharedPtr<ezVisualScriptInstanceDataMapping>& pInstanceDataMapping)
  : ezScriptInstance(inout_owner, pWorld)
  , m_pConstantDataStorage(pConstantDataStorage)
  , m_pInstanceDataMapping(pInstanceDataMapping)
{
  if (pInstanceDataDesc != nullptr)
  {
    m_pInstanceDataStorage = EZ_DEFAULT_NEW(ezVisualScriptDataStorage, pInstanceDataDesc);
    m_pInstanceDataStorage->AllocateStorage();

    for (auto& it : m_pInstanceDataMapping->m_Content)
    {
      auto& instanceData = it.Value();
      m_pInstanceDataStorage->SetDataFromVariant(instanceData.m_DataOffset, instanceData.m_DefaultValue, 0);
    }
  }
}

void ezVisualScriptInstance::ApplyParameters(const ezArrayMap<ezHashedString, ezVariant>& parameters)
{
  if (m_pInstanceDataMapping == nullptr)
    return;

  for (auto it : parameters)
  {
    ezVisualScriptInstanceData* pInstanceData = nullptr;
    if (m_pInstanceDataMapping->m_Content.TryGetValue(it.key, pInstanceData))
    {
      ezResult conversionStatus = EZ_FAILURE;
      ezVariantType::Enum targetType = ezVisualScriptDataType::GetVariantType(pInstanceData->m_DataOffset.GetType());

      ezVariant convertedValue = it.value.ConvertTo(targetType, &conversionStatus);
      if (conversionStatus.Failed())
      {
        ezLog::Error("Can't apply script parameter '{}' because the given value of type '{}' can't be converted the expected target type '{}'", it.key, it.value.GetType(), targetType);
        continue;
      }

      m_pInstanceDataStorage->SetDataFromVariant(pInstanceData->m_DataOffset, convertedValue, 0);
    }
  }
}
