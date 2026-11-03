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
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Flies, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Flies>)
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
// clang-format on

ezParticleBehavior_Flies::ezParticleBehavior_Flies() = default;
ezParticleBehavior_Flies::~ezParticleBehavior_Flies() = default;

void ezParticleBehavior_Flies::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const
{
  inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizer_ApplyVelocity>());
}

enum class BehaviorFliesVersion
{
  Version_0 = 0,
  Version_1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleBehavior_Flies::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)BehaviorFliesVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_fSpeed;
  inout_stream << m_fPathLength;
  inout_stream << m_fMaxEmitterDistance;
  inout_stream << m_MaxSteeringAngle;
}

void ezParticleBehavior_Flies::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)BehaviorFliesVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_fSpeed;
  inout_stream >> m_fPathLength;
  inout_stream >> m_fMaxEmitterDistance;
  inout_stream >> m_MaxSteeringAngle;
}

bool ezParticleBehavior_Flies::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleBehavior_Flies::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  ctx.CreateStream(ezParticleStreamNames::s_sPosition, ezProcessingStream::DataType::Float4, &data.m_pStreamPosition, false);
  ctx.CreateStream(ezParticleStreamNames::s_sVelocity, ezProcessingStream::DataType::Half4, &data.m_pStreamVelocity, false);

  data.m_TimeToChangeDir = ezTime::MakeZero();
}

void ezParticleBehavior_Flies::Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  EZ_PROFILE_SCOPE("PFX: Flies");

  const ezTime tCur = ctx.GetEffect()->GetTotalEffectLifeTime();
  const bool bChangeDirection = tCur >= data.m_TimeToChangeDir;

  if (!bChangeDirection)
    return;

  data.m_TimeToChangeDir = tCur + ezTime::MakeFromSeconds(m_fPathLength / m_fSpeed);

  const ezVec3 vEmitterPos = ctx.m_pSystem->GetTransform().m_vPosition;
  const float fMaxDistanceToEmitterSquared = ezMath::Square(m_fMaxEmitterDistance);

  ezProcessingStreamIterator<ezVec4> itPosition(data.m_pStreamPosition, uiNumElements, 0);
  ezProcessingStreamIterator<ezFloat16Vec4> itVelocity(data.m_pStreamVelocity, uiNumElements, 0);

  ezQuat qRot;

  while (!itPosition.HasReachedEnd())
  {
    // if (pLifeArray[i] == pMaxLifeArray[i])

    const ezVec3 vPartToEm = vEmitterPos - itPosition.Current().GetAsVec3();
    const float fDist = vPartToEm.GetLengthSquared();

    const ezVec4 vel = itVelocity.Current();
    const ezVec3 vDir(vel.x, vel.y, vel.z);
    const float fSpeed = vel.w;
    const ezVec3 vVelocity = vDir * fSpeed;

    if (fDist > fMaxDistanceToEmitterSquared)
    {
      ezVec3 vPivot;
      vPivot = vDir.CrossRH(vPartToEm);
      vPivot.NormalizeIfNotZero().IgnoreResult();

      qRot = ezQuat::MakeFromAxisAndAngle(vPivot, m_MaxSteeringAngle);

      const ezVec3 newVel = qRot * vVelocity;
      const float newSpeed = newVel.GetLength();
      const ezVec3 newDir = newSpeed > 0.0f ? newVel / newSpeed : ezVec3(0, 0, 1);

      itVelocity.Current() = ezVec4(newDir.x, newDir.y, newDir.z, newSpeed);
    }
    else
    {
      const ezVec3 newDir = ezVec3::MakeRandomDeviation(ctx.GetRNG(), m_MaxSteeringAngle, vDir);

      itVelocity.Current() = ezVec4(newDir.x, newDir.y, newDir.z, m_fSpeed);
    }

    itPosition.Advance();
    itVelocity.Advance();
  }
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Flies);
