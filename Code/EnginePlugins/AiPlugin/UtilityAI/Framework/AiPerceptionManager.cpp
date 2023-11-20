#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiControllerDesc.h>
#include <AiPlugin/UtilityAI/Framework/AiPerceptionManager.h>
#include <AiPlugin/UtilityAI/Framework/AiSensorManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiPerception, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiPerceptionGenerator, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiPerceptionTypeAttribute, 1, ezRTTIDefaultAllocator<ezAiPerceptionTypeAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("PerceptionType", m_sPerceptionType),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const ezRTTI* ezAiPerceptionGenerator::GetPerceptionType(const ezRTTI* pGeneratorType)
{
  const ezAiPerceptionTypeAttribute* pAttr = pGeneratorType->GetAttributeByType<ezAiPerceptionTypeAttribute>();
  if (pAttr == nullptr)
    return nullptr;

  const ezRTTI* pType = ezRTTI::FindTypeByName(pAttr->GetPerceptionTypeName());
  return (pType != nullptr && pType->IsDerivedFrom<ezAiPerception>()) ? pType : nullptr;
}

//////////////////////////////////////////////////////////////////////////

ezAiPerceptionManager::ezAiPerceptionManager() = default;
ezAiPerceptionManager::~ezAiPerceptionManager() = default;

void ezAiPerceptionManager::Initialize(const ezAiControllerDesc& desc, ezByteBlobPtr instanceData)
{
  m_pDesc = &desc;
  m_InstanceData = instanceData;
  m_NeededInUpdate.Clear();
  m_NeededInUpdate.SetCount(desc.GetPerceptionGenerators().GetCount(), 0);
}

void ezAiPerceptionManager::Deinitialize()
{
  m_pDesc = nullptr;
  m_InstanceData.Clear();
  m_NeededInUpdate.Clear();
}

void ezAiPerceptionManager::FlagAsNeeded(const ezAiInputPin& pin)
{
  for (ezUInt32 uiGeneratorIdx : pin.GetConnections())
  {
    m_NeededInUpdate[uiGeneratorIdx] = m_uiUpdateCount;
  }
}

void ezAiPerceptionManager::FlagNeededSensors(ezAiSensorManager& ref_SensorManager)
{
  const auto& generators = m_pDesc->GetPerceptionGenerators();
  for (ezUInt32 i = 0; i < generators.GetCount(); ++i)
  {
    if (m_NeededInUpdate[i] == m_uiUpdateCount)
    {
      generators[i].m_pGenerator->FlagNeededSensors(ref_SensorManager);
    }
  }
}

void ezAiPerceptionManager::UpdateNeededPerceptions(const ezAiContext& ctx)
{
  const auto& generators = m_pDesc->GetPerceptionGenerators();
  for (ezUInt32 i = 0; i < generators.GetCount(); ++i)
  {
    if (m_NeededInUpdate[i] == m_uiUpdateCount)
    {
      generators[i].m_pGenerator->UpdatePerceptions(ctx, GetInstanceData(i));
    }
  }

  ++m_uiUpdateCount;
}

bool ezAiPerceptionManager::HasPerceptions(const ezAiInputPin& pin) const
{
  const auto& generators = m_pDesc->GetPerceptionGenerators();
  for (ezUInt32 uiGeneratorIdx : pin.GetConnections())
  {
    if (generators[uiGeneratorIdx].m_pGenerator->HasPerceptions(GetInstanceData(uiGeneratorIdx)))
    {
      return true;
    }
  }

  return false;
}

void ezAiPerceptionManager::GetPerceptions(const ezAiInputPin& pin, ezDynamicArray<const ezAiPerception*>& out_Perceptions) const
{
  const auto& generators = m_pDesc->GetPerceptionGenerators();
  for (ezUInt32 uiGeneratorIdx : pin.GetConnections())
  {
    generators[uiGeneratorIdx].m_pGenerator->GetPerceptions(GetInstanceData(uiGeneratorIdx), out_Perceptions);
  }
}

void ezAiPerceptionManager::GetAllPerceptions(ezDynamicArray<const ezAiPerception*>& out_Perceptions) const
{
  const auto& generators = m_pDesc->GetPerceptionGenerators();
  for (ezUInt32 i = 0; i < generators.GetCount(); ++i)
  {
    generators[i].m_pGenerator->GetPerceptions(GetInstanceData(i), out_Perceptions);
  }
}

void ezAiPerceptionManager::HandleMessage(const ezAiContext& ctx, const ezMessage& msg)
{
  const auto& generators = m_pDesc->GetPerceptionGenerators();
  for (ezUInt32 i = 0; i < generators.GetCount(); ++i)
  {
    generators[i].m_pGenerator->HandleMessage(ctx, GetInstanceData(i), msg);
  }
}

void* ezAiPerceptionManager::GetInstanceData(ezUInt32 uiGeneratorIdx) const
{
  return ezInstanceDataAllocator::GetInstanceData(m_InstanceData, m_pDesc->GetPerceptionGenerators()[uiGeneratorIdx].m_uiInstanceDataOffset);
}
