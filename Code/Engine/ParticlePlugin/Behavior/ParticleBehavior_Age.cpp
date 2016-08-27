#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Age.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <CoreUtils/DataProcessing/Stream/StreamElementIterator.h>
#include <Core/World/World.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Age, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Age>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Age, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Age>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleBehaviorFactory_Age::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Age>();
}

void ezParticleBehaviorFactory_Age::CopyBehaviorProperties(ezParticleBehavior* pObject) const
{
}

void ezParticleBehaviorFactory_Age::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;
}

void ezParticleBehaviorFactory_Age::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

}

void ezParticleBehavior_Age::CreateRequiredStreams()
{
  CreateStream("LifeTime", ezStream::DataType::Float2, &m_pStreamLifeTime);

}

void ezParticleBehavior_Age::Process(ezUInt64 uiNumElements)
{
  ezStreamElementIterator<ezVec3> itLifeTime(m_pStreamLifeTime, uiNumElements);

  const float tDiff = (float)m_TimeDiff.GetSeconds();

  ezUInt64 uiCurElement = 0;
  while (!itLifeTime.HasReachedEnd())
  {
    itLifeTime.Current().x -= tDiff;

    if (itLifeTime.Current().x <= 0)
    {
      itLifeTime.Current().x = 0;

      /// \todo Get current element index from iterator ?
      m_pStreamGroup->RemoveElement(uiCurElement);
    }

    itLifeTime.Advance();
    ++uiCurElement;
  }
}

