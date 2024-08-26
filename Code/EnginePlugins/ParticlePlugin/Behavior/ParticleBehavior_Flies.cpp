#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Flies.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Flies, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Flies>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("FlySpeed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(0.2f), new ezClampValueAttribute(0.0f, 1000.0f)),
    EZ_MEMBER_PROPERTY("PathLength", m_fPathLength)->AddAttributes(new ezDefaultValueAttribute(0.2f), new ezClampValueAttribute(0.0f, 100.0f)),
    EZ_MEMBER_PROPERTY("MaxEmitterDistance", m_fMaxEmitterDistance)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 100.0f)),
    EZ_MEMBER_PROPERTY("MaxSteeringAngle", m_MaxSteeringAngle)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(30)), new ezClampValueAttribute(ezAngle::MakeFromDegree(1.0f), ezAngle::MakeFromDegree(180.0f))),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Flies, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Flies>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleBehaviorFactory_Flies::ezParticleBehaviorFactory_Flies() = default;
ezParticleBehaviorFactory_Flies::~ezParticleBehaviorFactory_Flies() = default;

const ezRTTI* ezParticleBehaviorFactory_Flies::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Flies>();
}

void ezParticleBehaviorFactory_Flies::CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const
{
  ezParticleBehavior_Flies* pBehavior = static_cast<ezParticleBehavior_Flies*>(pObject);

  pBehavior->m_fSpeed = m_fSpeed;
  pBehavior->m_fPathLength = m_fPathLength;
  pBehavior->m_fMaxEmitterDistance = m_fMaxEmitterDistance;
  pBehavior->m_MaxSteeringAngle = m_MaxSteeringAngle;
}

void ezParticleBehaviorFactory_Flies::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const
{
  inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_ApplyVelocity>());
}

enum class BehaviorFliesVersion
{
  Version_0 = 0,
  Version_1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleBehaviorFactory_Flies::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)BehaviorFliesVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_fSpeed;
  inout_stream << m_fPathLength;
  inout_stream << m_fMaxEmitterDistance;
  inout_stream << m_MaxSteeringAngle;
}

void ezParticleBehaviorFactory_Flies::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)BehaviorFliesVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_fSpeed;
  inout_stream >> m_fPathLength;
  inout_stream >> m_fMaxEmitterDistance;
  inout_stream >> m_MaxSteeringAngle;
}

void ezParticleBehavior_Flies::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity, false);

  m_TimeToChangeDir = ezTime::MakeZero();
}

void ezParticleBehavior_Flies::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: Flies");

  const ezTime tCur = GetOwnerEffect()->GetTotalEffectLifeTime();
  const bool bChangeDirection = tCur >= m_TimeToChangeDir;

  if (!bChangeDirection)
    return;

  m_TimeToChangeDir = tCur + ezTime::MakeFromSeconds(m_fPathLength / m_fSpeed);

  const ezVec3 vEmitterPos = GetOwnerSystem()->GetTransform().m_vPosition;
  const float fMaxDistanceToEmitterSquared = ezMath::Square(m_fMaxEmitterDistance);

  ezProcessingStreamIterator<ezVec4> itPosition(m_pStreamPosition, uiNumElements, 0);
  ezProcessingStreamIterator<ezVec3> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  ezQuat qRot;

  while (!itPosition.HasReachedEnd())
  {
    // if (pLifeArray[i] == pMaxLifeArray[i])

    const ezVec3 vPartToEm = vEmitterPos - itPosition.Current().GetAsVec3();
    const float fDist = vPartToEm.GetLengthSquared();
    const ezVec3 vVelocity = itVelocity.Current();
    ezVec3 vDir = vVelocity;
    vDir.NormalizeIfNotZero().IgnoreResult();

    if (fDist > fMaxDistanceToEmitterSquared)
    {
      ezVec3 vPivot;
      vPivot = vDir.CrossRH(vPartToEm);
      vPivot.NormalizeIfNotZero().IgnoreResult();

      qRot = ezQuat::MakeFromAxisAndAngle(vPivot, m_MaxSteeringAngle);

      itVelocity.Current() = qRot * vVelocity;
    }
    else
    {
      itVelocity.Current() = ezVec3::MakeRandomDeviation(GetRNG(), m_MaxSteeringAngle, vDir) * m_fSpeed;
    }

    itPosition.Advance();
    itVelocity.Advance();
  }
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Flies);
