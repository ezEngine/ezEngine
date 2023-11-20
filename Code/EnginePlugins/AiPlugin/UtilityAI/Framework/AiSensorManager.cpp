#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiControllerDesc.h>
#include <AiPlugin/UtilityAI/Framework/AiSensorManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiSensor, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAiSensorManager::ezAiSensorManager() = default;
ezAiSensorManager::~ezAiSensorManager() = default;

void ezAiSensorManager::Initialize(const ezAiControllerDesc& desc, ezByteBlobPtr instanceData)
{
  m_pDesc = &desc;
  m_InstanceData = instanceData;
  m_NeededInUpdate.Clear();
  m_NeededInUpdate.SetCount(desc.GetSensors().GetCount(), 0);
}

void ezAiSensorManager::Deinitialize()
{
  m_pDesc = nullptr;
  m_InstanceData.Clear();
  m_NeededInUpdate.Clear();
}

void ezAiSensorManager::FlagAsNeeded(ezUInt32 uiSensorIndex)
{
  if (uiSensorIndex != ezInvalidIndex)
  {
    m_NeededInUpdate[uiSensorIndex] = m_uiUpdateCount;
  }
}

void ezAiSensorManager::UpdateNeededSensors(const ezAiContext& ctx)
{
  const auto& sensors = m_pDesc->GetSensors();
  for (ezUInt32 i = 0; i < sensors.GetCount(); ++i)
  {
    if (m_NeededInUpdate[i] == m_uiUpdateCount)
    {
      sensors[i].m_pSensor->UpdateSensor(ctx, GetInstanceData(i));
    }
  }

  ++m_uiUpdateCount;
}

const ezAiSensor* ezAiSensorManager::GetSensor(ezUInt32 uiSensorIndex, const void*& out_pInstanceData) const
{
  if (uiSensorIndex == ezInvalidIndex)
  {
    out_pInstanceData = nullptr;
    return nullptr;
  }

  out_pInstanceData = GetInstanceData(uiSensorIndex);
  return m_pDesc->GetSensors()[uiSensorIndex].m_pSensor.Borrow();
}

void* ezAiSensorManager::GetInstanceData(ezUInt32 uiSensorIdx) const
{
  return ezInstanceDataAllocator::GetInstanceData(m_InstanceData, m_pDesc->GetSensors()[uiSensorIdx].m_uiInstanceDataOffset);
}
