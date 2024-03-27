#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Raycast.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_LastPosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Raycast, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Raycast>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Reaction", ezParticleRaycastHitReaction, m_Reaction),
    EZ_MEMBER_PROPERTY("BounceFactor", m_fBounceFactor)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("SizeFactor", m_fSizeFactor)->AddAttributes(new ezDefaultValueAttribute(0.1f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("OnCollideEvent", m_sOnCollideEvent),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Raycast, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Raycast>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezParticleRaycastHitReaction, 1)
  EZ_ENUM_CONSTANTS(ezParticleRaycastHitReaction::Bounce, ezParticleRaycastHitReaction::Die, ezParticleRaycastHitReaction::Stop)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezParticleBehaviorFactory_Raycast::ezParticleBehaviorFactory_Raycast() = default;
ezParticleBehaviorFactory_Raycast::~ezParticleBehaviorFactory_Raycast() = default;

const ezRTTI* ezParticleBehaviorFactory_Raycast::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Raycast>();
}

void ezParticleBehaviorFactory_Raycast::CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const
{
  ezParticleBehavior_Raycast* pBehavior = static_cast<ezParticleBehavior_Raycast*>(pObject);

  pBehavior->m_Reaction = m_Reaction;
  pBehavior->m_uiCollisionLayer = m_uiCollisionLayer;
  pBehavior->m_sOnCollideEvent = ezTempHashedString(m_sOnCollideEvent.GetData());
  pBehavior->m_fBounceFactor = m_fBounceFactor;
  pBehavior->m_fSizeFactor = m_fSizeFactor;

  pBehavior->m_pPhysicsModule = (ezPhysicsWorldModuleInterface*)pBehavior->GetOwnerSystem()->GetOwnerWorldModule()->GetCachedWorldModule(ezGetStaticRTTI<ezPhysicsWorldModuleInterface>());
}

enum class BehaviorRaycastVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added event
  Version_3, // added bounce factor
  Version_4, // added size factor

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void ezParticleBehaviorFactory_Raycast::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)BehaviorRaycastVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_uiCollisionLayer;
  inout_stream << m_sOnCollideEvent;
  inout_stream << m_Reaction;
  inout_stream << m_fBounceFactor;
  inout_stream << m_fSizeFactor;
}

void ezParticleBehaviorFactory_Raycast::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)BehaviorRaycastVersion::Version_Current, "Invalid version {0}", uiVersion);

  if (uiVersion >= 2)
  {
    inout_stream >> m_uiCollisionLayer;
    inout_stream >> m_sOnCollideEvent;
    inout_stream >> m_Reaction;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_fBounceFactor;
  }

  if (uiVersion >= 4)
  {
    inout_stream >> m_fSizeFactor;
  }
}

void ezParticleBehaviorFactory_Raycast::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const
{
  inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_ApplyVelocity>());
  inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_LastPosition>());
}

//////////////////////////////////////////////////////////////////////////

ezParticleBehavior_Raycast::ezParticleBehavior_Raycast()
{
  // do this right after ezParticleFinalizer_ApplyVelocity has run
  m_fPriority = 526.0f;
}

void ezParticleBehavior_Raycast::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("LastPosition", ezProcessingStream::DataType::Float3, &m_pStreamLastPosition, false);
  CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
}

void ezParticleBehavior_Raycast::QueryOptionalStreams()
{
  m_pStreamSize = GetOwnerSystem()->QueryStream("Size", ezProcessingStream::DataType::Half);
}

void ezParticleBehavior_Raycast::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: Raycast");

  const float tDiff = (float)m_TimeDiff.GetSeconds();

  ezProcessingStreamIterator<ezVec4> itPosition(m_pStreamPosition, uiNumElements, 0);
  ezProcessingStreamIterator<const ezVec3> itLastPosition(m_pStreamLastPosition, uiNumElements, 0);
  ezProcessingStreamIterator<ezVec3> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  ezFloat16 fDummySize = 0.0f;
  const ezFloat16* pSize = m_pStreamSize != nullptr ? m_pStreamSize->GetData<ezFloat16>() : &fDummySize;

  ezPhysicsCastResult hitResult;

  ezUInt32 i = 0;
  while (!itPosition.HasReachedEnd())
  {
    const ezVec3 vLastPos = itLastPosition.Current();
    const ezVec3 vCurPos = itPosition.Current().GetAsVec3();

    if (!vLastPos.IsZero())
    {
      const ezVec3 vChange = vCurPos - vLastPos;

      if (!vChange.IsZero(ezMath::DefaultEpsilon<float>()))
      {
        ezVec3 vDirection = vChange;

        const float fSize = ezMath::Max(*pSize * m_fSizeFactor, 0.01f);
        const float fMaxLen = vDirection.GetLengthAndNormalize() + fSize;

        ezPhysicsQueryParameters query(m_uiCollisionLayer);
        query.m_ShapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic;

        if (m_pPhysicsModule != nullptr && m_pPhysicsModule->Raycast(hitResult, vLastPos, vDirection, fMaxLen, query))
        {
          hitResult.m_vPosition -= vDirection * fSize;

          if (m_Reaction == ezParticleRaycastHitReaction::Bounce)
          {
            const ezVec3 vNewDir = vChange.GetReflectedVector(hitResult.m_vNormal) * m_fBounceFactor;

            if (vNewDir.GetLengthSquared() < ezMath::Square(0.01f))
            {
              itPosition.Current() = hitResult.m_vPosition.GetAsPositionVec4();
              itVelocity.Current().SetZero();
            }
            else
            {
              itPosition.Current() = ezVec3(hitResult.m_vPosition + vNewDir).GetAsVec4(0);
              itVelocity.Current() = vNewDir / tDiff;
            }
          }
          else if (m_Reaction == ezParticleRaycastHitReaction::Die)
          {
            /// \todo Get current element index from iterator ?
            m_pStreamGroup->RemoveElement(i);
          }
          else if (m_Reaction == ezParticleRaycastHitReaction::Stop)
          {
            itPosition.Current() = hitResult.m_vPosition.GetAsPositionVec4();
            itVelocity.Current().SetZero();
          }

          if (!m_sOnCollideEvent.IsEmpty())
          {
            ezParticleEvent e;
            e.m_EventType = m_sOnCollideEvent;
            e.m_vPosition = hitResult.m_vPosition;
            e.m_vNormal = hitResult.m_vNormal;
            e.m_vDirection = vDirection;

            GetOwnerEffect()->AddParticleEvent(e);
          }
        }

        if constexpr (false)
        {
          ezDebugRenderer::DrawLineSphere(m_pPhysicsModule->GetWorld(), ezBoundingSphere::MakeFromCenterAndRadius(itPosition.Current().GetAsVec3(), fSize), ezColor::Red);
        }
      }
    }

    itPosition.Advance();
    itLastPosition.Advance();
    itVelocity.Advance();

    if (m_pStreamSize != nullptr)
      ++pSize;

    ++i;
  }
}

void ezParticleBehavior_Raycast::RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<ezPhysicsWorldModuleInterface>();
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Raycast);
