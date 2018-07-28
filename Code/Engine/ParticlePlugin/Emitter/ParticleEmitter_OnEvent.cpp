#include <PCH.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_OnEvent.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Foundation/Time/Clock.h>
#include <Core/World/World.h>
#include <GameEngine/Curves/Curve1DResource.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <Foundation/Profiling/Profiling.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitterFactory_OnEvent, 1, ezRTTIDefaultAllocator<ezParticleEmitterFactory_OnEvent>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("EventName", m_sEventName),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEmitter_OnEvent, 1, ezRTTIDefaultAllocator<ezParticleEmitter_OnEvent>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleEmitterFactory_OnEvent::ezParticleEmitterFactory_OnEvent()
{
}


const ezRTTI* ezParticleEmitterFactory_OnEvent::GetEmitterType() const
{
  return ezGetStaticRTTI<ezParticleEmitter_OnEvent>();
}

void ezParticleEmitterFactory_OnEvent::CopyEmitterProperties(ezParticleEmitter* pEmitter0, bool bFirstTime) const
{
  ezParticleEmitter_OnEvent* pEmitter = static_cast<ezParticleEmitter_OnEvent*>(pEmitter0);

  pEmitter->m_sEventName = ezTempHashedString(m_sEventName.GetData());
}

void ezParticleEmitterFactory_OnEvent::QueryMaxParticleCount(ezUInt32& out_uiMaxParticlesAbs, ezUInt32& out_uiMaxParticlesPerSecond) const
{
  out_uiMaxParticlesAbs = 0;
  out_uiMaxParticlesPerSecond = 16; // some wild guess
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
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, true);
  CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity, true);
}

void ezParticleEmitter_OnEvent::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  EZ_PROFILE("PFX: Event Emitter");

  EZ_ASSERT_DEV(uiNumElements == m_Events.GetCount(), "Invalid spawn count");

  ezVec4* pPosition = m_pStreamPosition->GetWritableData<ezVec4>();
  ezVec3* pVelocity = m_pStreamVelocity->GetWritableData<ezVec3>();

  for (ezUInt32 i = 0; i < uiNumElements; ++i)
  {
    const ezUInt64 index = uiStartIndex + i;

    pPosition[index] = m_Events[i].m_vPosition.GetAsVec4(0);
    pVelocity[index] = m_Events[i].m_vDirection; /// \todo whatever
  }

  m_Events.Clear();
}

ezParticleEmitterState ezParticleEmitter_OnEvent::IsFinished()
{
  return ezParticleEmitterState::OnlyReacting;
}

ezUInt32 ezParticleEmitter_OnEvent::ComputeSpawnCount(const ezTime& tDiff)
{
  return m_Events.GetCount();
}

void ezParticleEmitter_OnEvent::ProcessEventQueue(ezParticleEventQueue queue)
{
  for (const ezParticleEvent& e : queue)
  {
    if (e.m_EventType == m_sEventName) // this is the event type we are waiting for!
    {
      if (m_Events.GetCount() == m_Events.GetCapacity())
        return;

      m_Events.PushBack(e);
    }
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_OnEvent);

