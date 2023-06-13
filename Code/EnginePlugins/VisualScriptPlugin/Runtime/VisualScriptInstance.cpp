#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

ezVisualScriptInstance::ezVisualScriptInstance(ezReflectedClass& inout_owner, ezWorld* pWorld, const ezSharedPtr<ezVisualScriptDataStorage>& pConstantDataStorage, const ezSharedPtr<const ezVisualScriptDataDescription>& pInstanceDataDesc)
  : ezScriptInstance(inout_owner, pWorld)
  , m_pConstantDataStorage(pConstantDataStorage)
{
  if (pInstanceDataDesc != nullptr)
  {
    m_pInstanceDataStorage = EZ_DEFAULT_NEW(ezVisualScriptDataStorage, pInstanceDataDesc);
    m_pInstanceDataStorage->AllocateStorage();
  }
}

void ezVisualScriptInstance::ApplyParameters(const ezArrayMap<ezHashedString, ezVariant>& parameters)
{
  for (auto it : parameters)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }
}
