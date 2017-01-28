#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_OnEvent.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Foundation/Time/Clock.h>
#include <Core/World/World.h>
#include <GameUtils/Curves/Curve1DResource.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <ParticlePlugin/Events/ParticleEvent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitterFactory_OnEvent, 1, ezRTTIDefaultAllocator<ezParticleEmitterFactory_OnEvent>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("EventName", m_sEventName),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitter_OnEvent, 1, ezRTTIDefaultAllocator<ezParticleEmitter_OnEvent>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleEmitterFactory_OnEvent::ezParticleEmitterFactory_OnEvent()
{
}


const ezRTTI* ezParticleEmitterFactory_OnEvent::GetEmitterType() const
{
  return ezGetStaticRTTI<ezParticleEmitter_OnEvent>();
}

void ezParticleEmitterFactory_OnEvent::CopyEmitterProperties(ezParticleEmitter* pEmitter0) const
{
  ezParticleEmitter_OnEvent* pEmitter = static_cast<ezParticleEmitter_OnEvent*>(pEmitter0);

  pEmitter->m_sEventName = ezTempHashedString(m_sEventName.GetData());
}

enum class EmitterOnEventVersion
{
  Version_0 = 0,
  Version_1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void ezParticleEmitterFactory_OnEvent::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)EmitterOnEventVersion::Version_Current;
  stream << uiVersion;

  // Version 1
  stream << m_sEventName;
}

void ezParticleEmitterFactory_OnEvent::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)EmitterOnEventVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_sEventName;
}


void ezParticleEmitter_OnEvent::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float3, &m_pStreamPosition, false);
  CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
}

void ezParticleEmitter_OnEvent::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  if (uiStartIndex + uiNumElements > m_Events.GetCount())
  {
    ezLog::Error("OnEvent emitter must be the only emitter on a particle system");
    return;
  }

  ezVec3* pPosition = m_pStreamPosition->GetWritableData<ezVec3>();
  ezVec3* pVelocity = m_pStreamVelocity->GetWritableData<ezVec3>();

  for (ezUInt32 i = 0; i < uiNumElements; ++i)
  {
    const ezUInt64 index = uiStartIndex + i;

    pPosition[index] = m_Events[i].m_vPosition;
    pVelocity[index] = m_Events[i].m_vDirection; /// \todo whatever
  }
}

ezParticleEmitterState ezParticleEmitter_OnEvent::IsFinished()
{
  return ezParticleEmitterState::OnlyReacting;
}

ezUInt32 ezParticleEmitter_OnEvent::ComputeSpawnCount(const ezTime& tDiff)
{
  return m_Events.GetCount();
}

void ezParticleEmitter_OnEvent::ProcessEventQueue(const ezParticleEventQueue* pQueue)
{
  if (pQueue->GetEventTypeHash() == m_sEventName.GetHash())
  {
    // this is the event type we are waiting for!

    // copy all the data. quite inefficient though, have to see whether that could be done better
    m_Events = pQueue->GetAllEvents();
  }
}

