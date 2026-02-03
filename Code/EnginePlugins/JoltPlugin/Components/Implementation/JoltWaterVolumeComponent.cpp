#include <JoltPlugin/JoltPluginPCH.h>

#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Actors/JoltTriggerComponent.h>
#include <JoltPlugin/Components/JoltWaterVolumeComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltWorldModule.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltWaterVolumeComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Extents", m_vExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(10.0f)), new ezClampValueAttribute(ezVec3(0.0f), ezVariant())),
    EZ_MEMBER_PROPERTY("Flow", m_vFlow),
    EZ_MEMBER_PROPERTY("NoiseStrength", m_fNoiseStrength)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_RESOURCE_MEMBER_PROPERTY("Surface", m_hSurface)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface", ezDependencyFlags::Package)),
    EZ_MEMBER_PROPERTY("Interaction", m_sInteraction)->AddAttributes(new ezDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgTriggerTriggered, OnMsgTriggerTriggered),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Jolt/Effects"),
    new ezBoxManipulatorAttribute("Extents", 1.0f, true),
    new ezBoxVisualizerAttribute("Extents", 1.0f, ezColorScheme::GetCategoryColor("Physics", ezColorScheme::CategoryColorUsage::ViewportIcon)),
    new ezDirectionVisualizerAttribute("Flow", 1.0f, ezColorScheme::GetCategoryColor("Physics", ezColorScheme::CategoryColorUsage::ViewportIcon)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

ezJoltWaterVolumeComponentManager::ezJoltWaterVolumeComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezJoltWaterVolumeComponent, ezBlockStorageType::FreeList>(pWorld)
  , m_Noise(12345)
{
}

ezJoltWaterVolumeComponentManager::~ezJoltWaterVolumeComponentManager() = default;

void ezJoltWaterVolumeComponentManager::UpdateWaterVolumes(ezTime deltaTime)
{
  EZ_PROFILE_SCOPE("UpdateWaterVolumes");

  auto pJoltSystem = GetWorld()->GetModule<ezJoltWorldModule>()->GetJoltSystem();

  for (auto it = GetComponents(0); it.IsValid(); it.Next())
  {
    if (it->IsActiveAndInitialized())
    {
      it->Update(*pJoltSystem, deltaTime);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

ezJoltWaterVolumeComponent::ezJoltWaterVolumeComponent() = default;
ezJoltWaterVolumeComponent::~ezJoltWaterVolumeComponent() = default;

void ezJoltWaterVolumeComponent::OnSimulationStarted()
{
  ezJoltTriggerComponent* pTriggerComponent = nullptr;
  if (GetOwner()->TryGetComponentOfBaseType(pTriggerComponent) == false)
  {
    ezLog::Warning("ezJoltWaterVolumeComponent requires an ezJoltTriggerComponent on the same game object.");
  }

  auto pJoltSystem = GetWorld()->GetOrCreateModule<ezJoltWorldModule>()->GetJoltSystem();
  UpdateWaterPlane(ezJoltConversionUtils::ToVec3(pJoltSystem->GetGravity()));
}

void ezJoltWaterVolumeComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_vFlow;
  s << m_fNoiseStrength;
  s << m_hSurface;
  s << m_sInteraction;
}

void ezJoltWaterVolumeComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  /*const ezUInt32 uiVersion =*/inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_vExtents;
  s >> m_vFlow;
  s >> m_fNoiseStrength;
  s >> m_hSurface;
  s >> m_sInteraction;
}

void ezJoltWaterVolumeComponent::OnMsgTriggerTriggered(ezMsgTriggerTriggered& msg)
{
  if (msg.m_TriggerState != ezTriggerState::Activated && msg.m_TriggerState != ezTriggerState::Deactivated)
    return;

  ezGameObject* pSubmergedObject = nullptr;
  if (!GetWorld()->TryGetObject(msg.m_hTriggeringObject, pSubmergedObject))
    return;

  ezJoltDynamicActorComponent* pActorComponent = nullptr;
  if (!pSubmergedObject->TryGetComponentOfBaseType(pActorComponent) || pActorComponent->GetKinematic())
    return;

  if (msg.m_TriggerState == ezTriggerState::Activated)
  {
    if (m_hSurface.IsValid() && m_sInteraction.IsEmpty() == false)
    {
      ezResourceLock<ezSurfaceResource> pSurface(m_hSurface, ezResourceAcquireMode::BlockTillLoaded);

      const ezVec3 vPos = m_SurfacePlane.ProjectOntoPlane(pSubmergedObject->GetGlobalPosition());
      const ezVec3 vNormal = m_SurfacePlane.m_vNormal;
      const ezVec3 vDirection = pSubmergedObject->GetLinearVelocity();

      pSurface->InteractWithSurface(GetWorld(), ezGameObjectHandle(), vPos, vNormal, vDirection, m_sInteraction, &GetOwner()->GetTeamID());
    }

    m_SubmergedActors.Insert(pActorComponent->GetHandle());
  }
  else if (msg.m_TriggerState == ezTriggerState::Deactivated)
  {
    m_SubmergedActors.Remove(pActorComponent->GetHandle());
  }
}

void ezJoltWaterVolumeComponent::CreateShapes(ezDynamicArray<ezJoltSubShape>& out_Shapes, const ezTransform& rootTransform, float fDensity, const ezJoltMaterial* pMaterial)
{
  // can't create boxes smaller than this
  ezVec3 size = m_vExtents * 0.5f;
  size.x = ezMath::Max(size.x, JPH::cDefaultConvexRadius);
  size.y = ezMath::Max(size.y, JPH::cDefaultConvexRadius);
  size.z = ezMath::Max(size.z, JPH::cDefaultConvexRadius);

  auto pNewShape = new JPH::BoxShape(ezJoltConversionUtils::ToVec3(size));
  pNewShape->AddRef();
  pNewShape->SetDensity(fDensity);
  pNewShape->SetUserData(reinterpret_cast<ezUInt64>(GetUserData()));
  pNewShape->SetMaterial(pMaterial);

  ezJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
  sub.m_pShape = pNewShape;
  sub.m_Transform = ezTransform::MakeLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
}

void ezJoltWaterVolumeComponent::Update(JPH::PhysicsSystem& joltSystem, ezTime deltaTime)
{
  const ezTransform globalTransform = GetOwner()->GetGlobalTransform();
  const ezVec3 vScaledExtents = m_vExtents.CompMul(globalTransform.m_vScale);

  if (vScaledExtents.x * vScaledExtents.y * vScaledExtents.z < ezMath::DefaultEpsilon<float>())
    return;

  const ezVec3 gravity = ezJoltConversionUtils::ToVec3(joltSystem.GetGravity());

  if (GetOwner()->IsDynamic() || m_vGravity.IsEqual(gravity, ezMath::DefaultEpsilon<float>()) == false)
  {
    UpdateWaterPlane(gravity);
  }

  const JPH::Vec3 flow = ezJoltConversionUtils::ToVec3(globalTransform.TransformDirection(m_vFlow));
  const float fDeltaTime = deltaTime.AsFloatInSeconds();

  const ezVec3 vSurfaceTangent = m_SurfacePlane.m_vNormal.GetOrthogonalVector().GetNormalized();
  const ezVec3 vSurfaceBitangent = m_SurfacePlane.m_vNormal.CrossRH(vSurfaceTangent).GetNormalized();

  m_fNoiseTime += fDeltaTime * 0.5f;
  if (m_fNoiseTime > 1000.0f)
    m_fNoiseTime -= 1000.0f;

  auto& noise = static_cast<ezJoltWaterVolumeComponentManager*>(GetOwningManager())->m_Noise;

  for (auto it : m_SubmergedActors)
  {
    ezJoltDynamicActorComponent* pActorComponent = nullptr;
    if (!GetWorld()->TryGetComponent(it, pActorComponent) || pActorComponent->IsActiveAndSimulating() == false)
      continue;

    JPH::BodyLockWrite lock(joltSystem.GetBodyLockInterface(), JPH::BodyID(pActorComponent->GetJoltBodyID()));
    JPH::Body& body = lock.GetBody();
    if (body.IsActive() && body.IsDynamic())
    {
      const ezVec3 pos = ezJoltConversionUtils::ToVec3(body.GetCenterOfMassPosition());
      ezVec3 surfacePosition = m_SurfacePlane.ProjectOntoPlane(pos);

      if (m_fNoiseStrength != 0.0f)
      {
        const float noisePosX = vSurfaceTangent.Dot(surfacePosition);
        const float noisePosY = vSurfaceBitangent.Dot(surfacePosition);

        ezSimdVec4f noisePos = ezSimdConversion::ToVec3(ezVec3(noisePosX, noisePosY, m_fNoiseTime));
        ezSimdVec4f noiseValue = noise.NoiseZeroToOne(ezSimdVec4f(noisePos.x()), ezSimdVec4f(noisePos.y()), ezSimdVec4f(noisePos.z()));

        surfacePosition += m_SurfacePlane.m_vNormal * (float(noiseValue.x()) * 2 - 1) * m_fNoiseStrength;

        body.ResetSleepTimer();
      }

      const JPH::Vec3 surfacePositionJolt = ezJoltConversionUtils::ToVec3(surfacePosition);
      const JPH::Vec3 surfaceNormal = ezJoltConversionUtils::ToVec3(m_SurfacePlane.m_vNormal);
      const float fBuoyancyFactor = pActorComponent->m_fBuoyancyFactor;

      body.ApplyBuoyancyImpulse(surfacePositionJolt, surfaceNormal, fBuoyancyFactor, 0.3f, 0.05f, flow, joltSystem.GetGravity(), fDeltaTime);
    }
  }
}

void ezJoltWaterVolumeComponent::UpdateWaterPlane(const ezVec3& vGravity)
{
  const ezTransform globalTransform = GetOwner()->GetGlobalTransform();
  const ezVec3 halfExtents = m_vExtents * 0.5f;
  const ezVec3 vGravityDir = vGravity.GetNormalized();
  float fMinDot = 1000.0f;

  for (ezUInt32 i = 0; i < 6; ++i)
  {
    ezVec3 vNormal = ezBasisAxis::GetBasisVector(static_cast<ezBasisAxis::Enum>(i));
    ezVec3 vPoint = vNormal.CompMul(halfExtents);
    vNormal = globalTransform.TransformDirection(vNormal).GetNormalized();

    float fDot = vNormal.Dot(vGravityDir);
    if (fDot < fMinDot)
    {
      vPoint = globalTransform.TransformPosition(vPoint);
      m_SurfacePlane = ezPlane::MakeFromNormalAndPoint(vNormal, vPoint);

      fMinDot = fDot;
    }
  }

  m_vGravity = vGravity;
}

EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltWaterVolumeComponent);
