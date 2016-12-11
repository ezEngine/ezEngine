#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/PhysXWorldModule.h>
#include <Core/World/World.h>

#include <PhysXPlugin/Components/PxStaticActorComponent.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/Shapes/PxShapeBoxComponent.h>
#include <PhysXPlugin/Shapes/PxShapeSphereComponent.h>
#include <PhysXPlugin/Shapes/PxShapeCapsuleComponent.h>
#include <PhysXPlugin/Shapes/PxShapeConvexComponent.h>
#include <PhysXPlugin/Components/PxCenterOfMassComponent.h>
#include <PhysXPlugin/Joints/PxDistanceJointComponent.h>
#include <PhysXPlugin/Joints/PxFixedJointComponent.h>
#include <PhysXPlugin/Components/PxCharacterControllerComponent.h>
#include <PhysXPlugin/Components/PxSettingsComponent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPhysXWorldModule, 1, ezRTTIDefaultAllocator<ezPhysXWorldModule>);
// no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE


PxFilterFlags ezPxFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
  // let triggers through
  if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
  {
    pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
    return PxFilterFlag::eDEFAULT;
  }

  pairFlags = (PxPairFlag::Enum)0;

  // trigger the contact callback for pairs (A,B) where
  // the filter mask of A contains the ID of B and vice versa.
  if ((filterData0.word0 & filterData1.word1) || (filterData1.word0 & filterData0.word1))
  {
    pairFlags |= PxPairFlag::eCONTACT_DEFAULT;
    return PxFilterFlag::eDEFAULT;
  }

  return PxFilterFlag::eKILL;
}


ezPhysXWorldModule::ezPhysXWorldModule()
{
  m_pPxScene = nullptr;

  SetGravity(ezVec3(0, 0, -10), ezVec3(0, 0, -12));
}

void ezPhysXWorldModule::SetGravity(const ezVec3& objectGravity, const ezVec3& characterGravity)
{
  m_vObjectGravity = objectGravity;
  m_vCharacterGravity = characterGravity;

  if (m_pPxScene)
  {
    m_pPxScene->setGravity(PxVec3(m_vObjectGravity.x, m_vObjectGravity.y, m_vObjectGravity.z));
  }
}

void ezPhysXWorldModule::InternalStartup()
{
  InternalReinit();

  GetWorld()->GetOrCreateComponentManager<ezPxStaticActorComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxDynamicActorComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxShapeBoxComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxShapeSphereComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxShapeCapsuleComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxShapeConvexComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxCenterOfMassComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxDistanceJointComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxFixedJointComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxCharacterControllerComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezPxSettingsComponentManager>()->SetUserData(this);

  m_AccumulatedTimeSinceUpdate.SetZero();

  PxSceneDesc desc = PxSceneDesc(PxTolerancesScale());
  desc.setToDefault(PxTolerancesScale());

  desc.gravity = PxVec3(m_vObjectGravity.x, m_vObjectGravity.y, m_vObjectGravity.z); /// \todo Scene settings

  m_pCPUDispatcher = PxDefaultCpuDispatcherCreate(4);
  desc.cpuDispatcher = m_pCPUDispatcher;
  desc.filterShader = ezPxFilterShader;

  EZ_ASSERT_DEV(desc.isValid(), "PhysX scene description is invalid");
  m_pPxScene = ezPhysX::GetSingleton()->GetPhysXAPI()->createScene(desc);

  m_pCharacterManager = PxCreateControllerManager(*m_pPxScene);

  EZ_ASSERT_ALWAYS(m_pPxScene != nullptr, "Creating the PhysX scene failed");
}

void ezPhysXWorldModule::InternalAfterWorldDestruction()
{
  //m_pCharacterManager->purgeControllers();
  m_pCharacterManager->release();
  m_pCharacterManager = nullptr;

  m_pPxScene->release();
  m_pPxScene = nullptr;

  m_pCPUDispatcher->release();
  m_pCPUDispatcher = nullptr;
}

void ezPhysXWorldModule::InternalUpdateBefore()
{
  if (!GetWorld()->GetWorldSimulationEnabled())
    return;

  EZ_LOCK(GetWorld()->GetWriteMarker());

  if (ezPxSettingsComponent* pSettings = GetWorld()->GetOrCreateComponentManager<ezPxSettingsComponentManager>()->GetSingletonComponent())
  {
    if (pSettings->IsModified())
    {
      SetGravity(pSettings->GetObjectGravity(), pSettings->GetCharacterGravity());

      pSettings->ResetModified();
    }
  }

  const ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();
  m_AccumulatedTimeSinceUpdate += tDiff;

  const ezTime tStep = ezTime::Seconds(1.0 / 60.0);

  while (m_AccumulatedTimeSinceUpdate >= tStep)
  {
    m_pPxScene->simulate((PxReal)tStep.GetSeconds());
    m_pPxScene->fetchResults(true);

    m_AccumulatedTimeSinceUpdate -= tStep;
  }

  // not sure where exactly this needs to go
  m_pCharacterManager->computeInteractions((float)tDiff.GetSeconds());
}


void ezPhysXWorldModule::InternalReinit()
{
  ezPhysX::GetSingleton()->LoadCollisionFilters();

}

void ezPxAllocatorCallback::VerifyAllocations()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  EZ_ASSERT_DEV(m_Allocations.IsEmpty(), "There are %u unfreed allocations", m_Allocations.GetCount());

  for (auto it = m_Allocations.GetIterator(); it.IsValid(); ++it)
  {
    const char* s = it.Value().GetData();
    ezLog::Info(s);
  }
#endif
}


bool ezPhysXWorldModule::CastRay(const ezVec3& vStart, const ezVec3& vDir, float fMaxLen, ezUInt8 uiCollisionLayer, ezVec3& out_vHitPos, ezVec3& out_vHitNormal, ezGameObjectHandle& out_hHitGameObject, ezSurfaceResourceHandle& out_hSurface)
{
  PxQueryFilterData filter;
  filter.data.setToDefault();
  filter.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

  filter.data.word0 = EZ_BIT(uiCollisionLayer);
  filter.data.word1 = ezPhysX::GetSingleton()->GetCollisionFilterConfig().GetFilterMask(uiCollisionLayer);
  filter.data.word2 = 0;
  filter.data.word3 = 0;

  ezPxQueryFilter QueryFilter;

  PxRaycastHit hit;
  if (GetPxScene()->raycastSingle(reinterpret_cast<const PxVec3&>(vStart), reinterpret_cast<const PxVec3&>(vDir),
                                  fMaxLen, PxHitFlag::ePOSITION | PxHitFlag::eNORMAL, hit,
                                  filter, &QueryFilter))
  {
    EZ_ASSERT_DEBUG(hit.shape != nullptr, "Raycast should have hit a shape");

    out_vHitPos = reinterpret_cast<const ezVec3&>(hit.position);
    out_vHitNormal = reinterpret_cast<const ezVec3&>(hit.normal);

    ezGameObject* pGameObject = reinterpret_cast<ezGameObject*>(hit.shape->userData);
    EZ_ASSERT_DEBUG(pGameObject != nullptr, "Shape should have set a game object as user data");

    out_hHitGameObject = pGameObject->GetHandle();

    EZ_ASSERT_DEBUG(!out_vHitPos.IsNaN(), "Raycast hit Position is NaN");
    EZ_ASSERT_DEBUG(!out_vHitNormal.IsNaN(), "Raycast hit Normal is NaN");

    auto pMaterial = hit.shape->getMaterialFromInternalFaceIndex(hit.faceIndex);

    if (pMaterial)
    {
      ezSurfaceResource* pSurface = reinterpret_cast<ezSurfaceResource*>(pMaterial->userData);

      out_hSurface = ezSurfaceResourceHandle(pSurface);
    }

    return true;
  }

  return false;
}

class ezPxSweepCallback : public PxSweepCallback
{
public:
  ezPxSweepCallback()
    : PxSweepCallback(nullptr, 0)
  {

  }

  virtual PxAgain processTouches(const PxSweepHit* buffer, PxU32 nbHits) override
  {
    return false;
  }
};

bool ezPhysXWorldModule::SweepTestCapsule(const ezTransform& start, const ezVec3& vDir, float fCapsuleRadius, float fCapsuleHeight, float fDistance, ezUInt8 uiCollisionLayer, float& out_fDistance, ezVec3& out_Position, ezVec3& out_Normal)
{
  PxQueryFilterData filter;
  filter.data.setToDefault();
  filter.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

  filter.data.word0 = EZ_BIT(uiCollisionLayer);
  filter.data.word1 = ezPhysX::GetSingleton()->GetCollisionFilterConfig().GetFilterMask(uiCollisionLayer);
  filter.data.word2 = 0;
  filter.data.word3 = 0;

  ezPxQueryFilter QueryFilter;

  PxCapsuleGeometry capsule;
  capsule.radius = fCapsuleRadius;
  capsule.halfHeight = fCapsuleHeight * 0.5f;
  EZ_ASSERT_DEBUG(capsule.isValid(), "Invalid capsule parameter. Radius = %.2f, Height = %.2f", fCapsuleRadius, fCapsuleHeight);

  ezQuat qFixRot;
  qFixRot.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90.0f));

  ezQuat qRot;
  qRot.SetFromMat3(start.m_Rotation);
  qRot = qFixRot * qRot;

  PxTransform pxt;
  pxt.p = *reinterpret_cast<const PxVec3*>(&start.m_vPosition);
  pxt.q = PxQuat(qRot.v.x, qRot.v.y, qRot.v.z, qRot.w);


  ezPxSweepCallback closestHit;

  PxRaycastHit hit;
  if (!GetPxScene()->sweep(capsule, pxt, *reinterpret_cast<const PxVec3*>(&vDir), fDistance, closestHit, PxHitFlag::eDEFAULT, filter, &QueryFilter))
    return false;

  out_fDistance = closestHit.block.distance;
  out_Position = *reinterpret_cast<ezVec3*>(&closestHit.block.position);
  out_Normal = *reinterpret_cast<ezVec3*>(&closestHit.block.normal);
  //closestHit.block.shape;
  //closestHit.block.actor;


  return true;
}





