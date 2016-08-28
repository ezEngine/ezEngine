#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Raycast.h>
#include <GameUtils/Interfaces/PhysicsWorldModule.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>
#include <CoreUtils/DataProcessing/Stream/StreamElementIterator.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Raycast, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Raycast>)
//{
//  EZ_BEGIN_PROPERTIES
//  {
//  }
//  EZ_END_PROPERTIES
//}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Raycast, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Raycast>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleBehaviorFactory_Raycast::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Raycast>();
}

void ezParticleBehaviorFactory_Raycast::CopyBehaviorProperties(ezParticleBehavior* pObject) const
{
}

void ezParticleBehaviorFactory_Raycast::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;
}

void ezParticleBehaviorFactory_Raycast::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;
}

void ezParticleBehavior_Raycast::AfterPropertiesConfigured()
{
  m_pPhysicsModule = static_cast<ezPhysicsWorldModuleInterface*>(ezWorldModule::FindModule(GetOwnerSystem()->GetWorld(), ezPhysicsWorldModuleInterface::GetStaticRTTI()));
}


void ezParticleBehavior_Raycast::CreateRequiredStreams()
{
  CreateStream("Position", ezStream::DataType::Float3, &m_pStreamPosition);
  CreateStream("Velocity", ezStream::DataType::Float3, &m_pStreamVelocity);
}

void ezParticleBehavior_Raycast::Process(ezUInt64 uiNumElements)
{
  const float tDiff = (float)m_TimeDiff.GetSeconds();

  ezStreamElementIterator<ezVec3> itPosition(m_pStreamPosition, uiNumElements);
  ezStreamElementIterator<ezVec3> itVelocity(m_pStreamVelocity, uiNumElements);

  ezGameObjectHandle hHitObj;
  ezSurfaceResourceHandle hHitSurface;

  while (!itPosition.HasReachedEnd())
  {
    const ezVec3 vVelocity = itVelocity.Current();

    if (!vVelocity.IsZero(0.001f))
    {
      ezVec3 vDirection = vVelocity * tDiff;
      const float fMaxLen = vDirection.GetLengthAndNormalize();

      ezVec3 vHitPos, vHitNorm;
      if (m_pPhysicsModule->CastRay(itPosition.Current(), vDirection, fMaxLen, 0, vHitPos, vHitNorm, hHitObj, hHitSurface))
      {
        itPosition.Current() = vHitPos + vHitNorm * 0.05f;
        itVelocity.Current() = vVelocity.GetReflectedVector(vHitNorm);
      }
    }

    itPosition.Advance();
    itVelocity.Advance();
  }
}

