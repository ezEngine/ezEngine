#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Velocity.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezVelocityChangeMode, 1)
  EZ_ENUM_CONSTANT(ezVelocityChangeMode::CustomCurve),
  EZ_ENUM_CONSTANT(ezVelocityChangeMode::SharedCurve),
  EZ_ENUM_CONSTANT(ezVelocityChangeMode::Friction),
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Velocity, 2, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Velocity>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("ChangeSpeedWith", ezVelocityChangeMode, m_ChangeSpeedWith),
    EZ_MEMBER_PROPERTY("Friction", m_fFriction)->AddAttributes(new ezClampValueAttribute(0.0f, 100.0f)),
    EZ_MEMBER_PROPERTY("SpeedCurve", m_SpeedCurve),
    EZ_RESOURCE_MEMBER_PROPERTY("SharedSpeedCurve", m_hSpeedSharedCurve)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
    EZ_MEMBER_PROPERTY("SpeedCurveOffset", m_fSpeedCurveOffset)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("SpeedCurveScale", m_fSpeedCurveScale)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),

  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Velocity, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Velocity>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleBehaviorFactory_Velocity::ezParticleBehaviorFactory_Velocity() = default;
ezParticleBehaviorFactory_Velocity::~ezParticleBehaviorFactory_Velocity() = default;

const ezRTTI* ezParticleBehaviorFactory_Velocity::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Velocity>();
}

void ezParticleBehaviorFactory_Velocity::CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const
{
  ezParticleBehavior_Velocity* pBehavior = static_cast<ezParticleBehavior_Velocity*>(pObject);

  pBehavior->m_fFriction = m_fFriction;
  pBehavior->m_ChangeSpeedWith = m_ChangeSpeedWith;
  pBehavior->m_fSpeedCurveOffset = m_fSpeedCurveOffset;
  pBehavior->m_fSpeedCurveScale = m_fSpeedCurveScale;
  pBehavior->m_pCurve = &m_RuntimeSpeedCurve;
}

enum class BehaviorVelocityVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added rise speed and acceleration
  Version_3, // added wind influence
  Version_4, // added speed curves
  Version_5, // removed wind influence (migrated to Wind behavior)
  Version_6, // removed rise speed

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleBehaviorFactory_Velocity::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)BehaviorVelocityVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_fFriction;

  // Version 4
  inout_stream << m_ChangeSpeedWith;
  inout_stream << m_hSpeedSharedCurve;
  inout_stream << m_fSpeedCurveOffset;
  inout_stream << m_fSpeedCurveScale;

  m_SpeedCurve.ConvertToRuntimeData(m_RuntimeSpeedCurve);
  m_RuntimeSpeedCurve.SortControlPoints();
  m_RuntimeSpeedCurve.Save(inout_stream);
}

void ezParticleBehaviorFactory_Velocity::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)BehaviorVelocityVersion::Version_Current, "Invalid version {0}", uiVersion);

  if (uiVersion < 6)
  {
    float fRiseSpeed;
    inout_stream >> fRiseSpeed;
  }

  inout_stream >> m_fFriction;

  if (uiVersion >= 3 && uiVersion < 5)
  {
    // wind influence was removed in version 5, but still need to read it from old files
    float fWindInfluence = 0;
    inout_stream >> fWindInfluence;
  }

  if (uiVersion >= 4)
  {
    inout_stream >> m_ChangeSpeedWith;
    inout_stream >> m_hSpeedSharedCurve;
    inout_stream >> m_fSpeedCurveOffset;
    inout_stream >> m_fSpeedCurveScale;

    m_RuntimeSpeedCurve.Load(inout_stream);
    m_RuntimeSpeedCurve.SortControlPoints(); // also updates the aabb
    m_RuntimeSpeedCurve.CreateLinearApproximation();

    if (m_ChangeSpeedWith == ezVelocityChangeMode::SharedCurve && m_hSpeedSharedCurve.IsValid())
    {
      ezResourceLock<ezCurve1DResource> pCurveResource(m_hSpeedSharedCurve, ezResourceAcquireMode::BlockTillLoaded);
      if (pCurveResource.GetAcquireResult() == ezResourceAcquireResult::Final && !pCurveResource->GetDescriptor().m_Curves.IsEmpty())
      {
        m_RuntimeSpeedCurve = pCurveResource->GetDescriptor().m_Curves[0];
      }
    }
  }
}

void ezParticleBehaviorFactory_Velocity::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const
{
  inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_ApplyVelocity>());
}

void ezParticleBehavior_Velocity::CreateRequiredStreams()
{
  CreateStream("Velocity", ezProcessingStream::DataType::Half4, &m_pStreamVelocity, false);

  if ((m_ChangeSpeedWith == ezVelocityChangeMode::CustomCurve || m_ChangeSpeedWith == ezVelocityChangeMode::SharedCurve) && m_pCurve && !m_pCurve->IsEmpty())
  {
    CreateStream("LifeTime", ezProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  }
}

void ezParticleBehavior_Velocity::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: Velocity");

  auto pOwner = GetOwnerEffect();

  const float tDiff = (float)m_TimeDiff.GetSeconds();

  ezProcessingStreamIterator<ezFloat16Vec4> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  // Handle curve-based speed changes
  if ((m_ChangeSpeedWith == ezVelocityChangeMode::CustomCurve || m_ChangeSpeedWith == ezVelocityChangeMode::SharedCurve) && m_pStreamLifeTime)
  {
    double fMinX, fMaxX;
    m_pCurve->QueryExtents(fMinX, fMaxX);

    // make sure the curve has a length of at least 1
    fMinX = ezMath::Min(fMinX, 0.0);
    fMaxX = ezMath::Max(fMaxX, 1.0);

    ezProcessingStreamIterator<ezFloat16Vec2> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);

    while (!itVelocity.HasReachedEnd())
    {
      const float fLifeTimeFraction = itLifeTime.Current().x * itLifeTime.Current().y;

      const double evalPos = ezMath::Lerp(fMaxX, fMinX, fLifeTimeFraction);
      const float val = (float)m_pCurve->Evaluate(evalPos);

      ezVec4 vel = itVelocity.Current();
      vel.w = m_fSpeedCurveOffset + val * m_fSpeedCurveScale;
      itVelocity.Current() = vel;

      itVelocity.Advance();
      itLifeTime.Advance();
    }
  }
  // Handle friction or no speed changes
  else
  {
    const float fFriction = (m_ChangeSpeedWith == ezVelocityChangeMode::Friction) ? ezMath::Clamp(m_fFriction, 0.0f, 100.0f) : 0.0f;
    const float fFrictionFactor = ezMath::Pow(0.5f, tDiff * fFriction);

    while (!itVelocity.HasReachedEnd())
    {
      ezVec4 vel = itVelocity.Current();
      vel.w *= fFrictionFactor;
      itVelocity.Current() = vel;

      itVelocity.Advance();
    }
  }
}

void ezParticleBehavior_Velocity::RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<ezPhysicsWorldModuleInterface>();
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Velocity);
