#include <ParticlePluginPCH.h>

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
    EZ_MEMBER_PROPERTY("MaxSteeringAngle", m_MaxSteeringAngle)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(30)), new ezClampValueAttribute(ezAngle::Degree(1.0f), ezAngle::Degree(180.0f))),
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

void ezParticleBehaviorFactory_Flies::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_FinalizerDeps) const
{
  inout_FinalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_ApplyVelocity>());
}

enum class BehaviorFliesVersion
{
  Version_0 = 0,
  Version_1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleBehaviorFactory_Flies::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)BehaviorFliesVersion::Version_Current;
  stream << uiVersion;

  stream << m_fSpeed;
  stream << m_fPathLength;
  stream << m_fMaxEmitterDistance;
  stream << m_MaxSteeringAngle;
}

void ezParticleBehaviorFactory_Flies::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)BehaviorFliesVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_fSpeed;
  stream >> m_fPathLength;
  stream >> m_fMaxEmitterDistance;
  stream >> m_MaxSteeringAngle;
}

void ezParticleBehavior_Flies::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity, false);

  m_TimeToChangeDir.SetZero();
}

void ezParticleBehavior_Flies::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: Flies");

  const ezTime tCur = GetOwnerEffect()->GetWorld()->GetClock().GetAccumulatedTime();
  const bool bChangeDirection = tCur >= m_TimeToChangeDir;

  if (!bChangeDirection)
    return;

  m_TimeToChangeDir = tCur + ezTime::Seconds(m_fPathLength / m_fSpeed);

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
    vDir.NormalizeIfNotZero();

    if (fDist > fMaxDistanceToEmitterSquared)
    {
      ezVec3 vPivot;
      vPivot = vDir.CrossRH(vPartToEm);
      vPivot.NormalizeIfNotZero();

      qRot.SetFromAxisAndAngle(vPivot, m_MaxSteeringAngle);

      itVelocity.Current() = qRot * vVelocity;
    }
    else
    {
      itVelocity.Current() = ezVec3::CreateRandomDeviation(GetRNG(), m_MaxSteeringAngle, vDir) * m_fSpeed;
    }

    itPosition.Advance();
    itVelocity.Advance();
  }
}
