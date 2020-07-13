#include <PhysXPluginPCH.h>

#include <Components/PxStaticActorComponent.h>
#include <Components/PxTriggerComponent.h>
#include <Core/Messages/CollisionMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/World/World.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/Prefabs/PrefabResource.h>
#include <Joints/PxJointComponent.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/Components/PxSettingsComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <PxArticulation.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <Shapes/PxShapeBoxComponent.h>

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezPhysXWorldModule);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPhysXWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

namespace
{
  class ezPxTask final : public ezTask
  {
  public:
    virtual void Execute() override
    {
      m_pTask->run();
      m_pTask->release();
      m_pTask = nullptr;
    }

    PxBaseTask* m_pTask;
  };

  class ezPxCpuDispatcher : public PxCpuDispatcher
  {
  public:
    ezPxCpuDispatcher() {}

    virtual void submitTask(PxBaseTask& task) override
    {
      ezSharedPtr<ezTask> pTask;

      {
        EZ_LOCK(m_Mutex);

        if (m_FreeTasks.IsEmpty())
        {
          m_TaskStorage.PushBack(EZ_DEFAULT_NEW(ezPxTask));
          pTask = m_TaskStorage.PeekBack();
        }
        else
        {
          pTask = m_FreeTasks.PeekBack();
          m_FreeTasks.PopBack();
        }
      }

      pTask->ConfigureTask(task.getName(), ezTaskNesting::Never, [this](const ezSharedPtr<ezTask>& pTask) { FinishTask(pTask); });
      static_cast<ezPxTask*>(pTask.Borrow())->m_pTask = &task;
      ezTaskSystem::StartSingleTask(pTask, ezTaskPriority::EarlyThisFrame);
    }

    virtual PxU32 getWorkerCount() const override { return ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::ShortTasks); }

    void FinishTask(const ezSharedPtr<ezTask>& pTask)
    {
      EZ_LOCK(m_Mutex);
      m_FreeTasks.PushBack(pTask);
    }

    ezMutex m_Mutex;
    ezDeque<ezSharedPtr<ezTask>, ezStaticAllocatorWrapper> m_TaskStorage;
    ezDynamicArray<ezSharedPtr<ezTask>, ezStaticAllocatorWrapper> m_FreeTasks;
  };

  static ezPxCpuDispatcher s_CpuDispatcher;

  PxFilterFlags ezPxFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1,
    PxFilterData filterData1, PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
  {
    const bool kinematic0 = PxFilterObjectIsKinematic(attributes0);
    const bool kinematic1 = PxFilterObjectIsKinematic(attributes1);

    const bool static0 = PxGetFilterObjectType(attributes0) == PxFilterObjectType::eRIGID_STATIC;
    const bool static1 = PxGetFilterObjectType(attributes1) == PxFilterObjectType::eRIGID_STATIC;

    if ((kinematic0 || kinematic1) && (static0 || static1))
    {
      return PxFilterFlag::eSUPPRESS;
    }

    // even though the documentation says that bodies in an articulation that are connected by a joint are generally ignored,
    // this is currently not true in reality
    // thus all shapes in an articulation (ragdoll) will collide with each other, which makes it impossible to build a proper ragdoll
    // the only way to prevent this, seems to be to disable ALL self-collision, unfortunately this also disables collisions with all other
    // articulations
    // TODO: this needs to be revisited with later PhysX versions
    if (PxGetFilterObjectType(attributes0) == PxFilterObjectType::eARTICULATION &&
        PxGetFilterObjectType(attributes1) == PxFilterObjectType::eARTICULATION)
    {
      return PxFilterFlag::eSUPPRESS;
    }

    pairFlags = (PxPairFlag::Enum)0;

    // trigger the contact callback for pairs (A,B) where
    // the filter mask of A contains the ID of B and vice versa.
    if ((filterData0.word0 & filterData1.word1) || (filterData1.word0 & filterData0.word1))
    {
      // let triggers through
      // note that triggers are typically kinematic
      // same for character controllers
      if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
      {
        pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
        return PxFilterFlag::eDEFAULT;
      }

      // set when "report contacts" is enabled on a shape
      if (filterData0.word3 != 0 || filterData1.word3 != 0)
      {
        pairFlags |= (PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_PERSISTS | PxPairFlag::eNOTIFY_CONTACT_POINTS);
      }

      // if neither object is a trigger and both are kinematic, just suppress the contact
      if (kinematic0 && kinematic1)
      {
        return PxFilterFlag::eSUPPRESS;
      }

      pairFlags |= PxPairFlag::eCONTACT_DEFAULT;
      pairFlags |= PxPairFlag::eDETECT_CCD_CONTACT;

      return PxFilterFlag::eDEFAULT;
    }

    return PxFilterFlag::eKILL;
  }

  class ezPxRaycastCallback : public PxRaycastCallback
  {
  public:
    ezPxRaycastCallback()
      : PxRaycastCallback(nullptr, 0)
    {
    }

    virtual PxAgain processTouches(const PxRaycastHit* buffer, PxU32 nbHits) override { return false; }
  };

  class ezPxSweepCallback : public PxSweepCallback
  {
  public:
    ezPxSweepCallback()
      : PxSweepCallback(nullptr, 0)
    {
    }

    virtual PxAgain processTouches(const PxSweepHit* buffer, PxU32 nbHits) override { return false; }
  };

  class ezPxOverlapCallback : public PxOverlapCallback
  {
  public:
    ezPxOverlapCallback()
      : PxOverlapCallback(nullptr, 0)
    {
    }

    virtual PxAgain processTouches(const PxOverlapHit* buffer, PxU32 nbHits) override { return false; }
  };

  template <typename T>
  void FillHitResult(const T& hit, ezPhysicsCastResult& out_Result)
  {
    PxShape* pHitShape = hit.shape;
    EZ_ASSERT_DEBUG(pHitShape != nullptr, "Raycast should have hit a shape");

    out_Result.m_vPosition = ezPxConversionUtils::ToVec3(hit.position);
    out_Result.m_vNormal = ezPxConversionUtils::ToVec3(hit.normal);
    out_Result.m_fDistance = hit.distance;
    EZ_ASSERT_DEBUG(!out_Result.m_vPosition.IsNaN(), "Raycast hit Position is NaN");
    EZ_ASSERT_DEBUG(!out_Result.m_vNormal.IsNaN(), "Raycast hit Normal is NaN");

    if (ezComponent* pShapeComponent = ezPxUserData::GetComponent(pHitShape->userData))
    {
      out_Result.m_hShapeObject = pShapeComponent->GetOwner()->GetHandle();
      out_Result.m_uiShapeId = pHitShape->getQueryFilterData().word2;
    }

    if (ezComponent* pActorComponent = ezPxUserData::GetComponent(pHitShape->getActor()->userData))
    {
      out_Result.m_hActorObject = pActorComponent->GetOwner()->GetHandle();
    }

    if (PxMaterial* pMaterial = pHitShape->getMaterialFromInternalFaceIndex(hit.faceIndex))
    {
      ezSurfaceResource* pSurface = ezPxUserData::GetSurfaceResource(pMaterial->userData);

      out_Result.m_hSurface = ezSurfaceResourceHandle(pSurface);
    }
  }

  static thread_local ezDynamicArray<PxOverlapHit, ezStaticAllocatorWrapper> g_OverlapHits;
  static thread_local ezDynamicArray<PxRaycastHit, ezStaticAllocatorWrapper> g_RaycastHits;
} // namespace

EZ_DEFINE_AS_POD_TYPE(PxOverlapHit);
EZ_DEFINE_AS_POD_TYPE(PxRaycastHit);

//////////////////////////////////////////////////////////////////////////

class ezPxSimulationEventCallback : public PxSimulationEventCallback
{
public:
  struct InteractionContact
  {
    ezVec3 m_vPosition;
    ezVec3 m_vNormal;
    ezSurfaceResource* m_pSurface;
    ezTempHashedString m_sInteraction;
    float m_fImpulseSqr;
  };

  struct SlidingInfo
  {
    bool m_bStillSliding = false;
    bool m_bStartedSliding = false;

    bool m_bStillRolling = false;
    bool m_bStartedRolling = false;

    ezVec3 m_vPosition;
    ezVec3 m_vNormal;
    ezGameObjectHandle m_hSlidePrefab;
    ezGameObjectHandle m_hRollPrefab;
  };

  ezDynamicArray<InteractionContact> m_InteractionContacts;
  ezMap<PxRigidDynamic*, SlidingInfo> m_SlidingActors;
  ezDeque<PxConstraint*> m_BrokenConstraints;

  virtual void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override
  {
    for (ezUInt32 i = 0; i < count; ++i)
    {
      m_BrokenConstraints.PushBack(constraints[i].constraint);
    }
  }

  virtual void onWake(PxActor** actors, PxU32 count) override
  {
    // TODO: send a "actor awake" message
  }

  virtual void onSleep(PxActor** actors, PxU32 count) override
  {
    // TODO: send a "actor asleep" message
  }

  virtual void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override
  {
    if (pairHeader.flags & (PxContactPairHeaderFlag::eREMOVED_ACTOR_0 | PxContactPairHeaderFlag::eREMOVED_ACTOR_1))
    {
      return;
    }

    bool bSendContactReport = false;

    for (ezUInt32 uiPairIndex = 0; uiPairIndex < nbPairs; ++uiPairIndex)
    {
      const PxContactPair& pair = pairs[uiPairIndex];

      PxContactPairPoint contactPointBuffer[16];
      const ezUInt32 uiNumContactPoints = pair.extractContacts(contactPointBuffer, 16);

      ezBitflags<ezOnPhysXContact> ContactFlags0;
      ContactFlags0.SetValue(pair.shapes[0]->getSimulationFilterData().word3);
      ezBitflags<ezOnPhysXContact> ContactFlags1;
      ContactFlags1.SetValue(pair.shapes[1]->getSimulationFilterData().word3);

      ezBitflags<ezOnPhysXContact> CombinedContactFlags;
      CombinedContactFlags.SetValue(pair.shapes[0]->getSimulationFilterData().word3 | pair.shapes[1]->getSimulationFilterData().word3);

      bSendContactReport = bSendContactReport || CombinedContactFlags.IsSet(ezOnPhysXContact::SendReportMsg);

      if (CombinedContactFlags.IsAnySet(ezOnPhysXContact::ImpactReactions | ezOnPhysXContact::SlideReactions |
                                        ezOnPhysXContact::RollXReactions | ezOnPhysXContact::RollYReactions | ezOnPhysXContact::RollZReactions))
      {
        ezVec3 vAvgPos(0);
        ezVec3 vAvgNormal(0);
        float fMaxImpactSqr = 0.0f;

        for (ezUInt32 uiContactPointIndex = 0; uiContactPointIndex < uiNumContactPoints; ++uiContactPointIndex)
        {
          const PxContactPairPoint& point = contactPointBuffer[uiContactPointIndex];

          vAvgPos += ezPxConversionUtils::ToVec3(point.position);
          vAvgNormal += ezPxConversionUtils::ToVec3(point.normal);
          fMaxImpactSqr = ezMath::Max(fMaxImpactSqr, point.impulse.magnitudeSquared());
        }

        vAvgPos /= (float)uiNumContactPoints;
        vAvgNormal.NormalizeIfNotZero(ezVec3::ZeroVector());

        if (CombinedContactFlags.IsAnySet(ezOnPhysXContact::SlideReactions | ezOnPhysXContact::RollXReactions | ezOnPhysXContact::RollYReactions | ezOnPhysXContact::RollZReactions) &&
            !pair.flags.isSet(PxContactPairFlag::eACTOR_PAIR_HAS_FIRST_TOUCH))
        {
          ezVec3 vVelocity0(0.0f);
          ezVec3 vVelocity1(0.0f);
          PxRigidDynamic* pRigid0 = nullptr;
          PxRigidDynamic* pRigid1 = nullptr;

          if (pairHeader.actors[0]->getType() == PxActorType::eRIGID_DYNAMIC)
          {
            pRigid0 = static_cast<PxRigidDynamic*>(pairHeader.actors[0]);

            vVelocity0 = ezPxConversionUtils::ToVec3(pRigid0->getLinearVelocity());
          }

          if (pairHeader.actors[1]->getType() == PxActorType::eRIGID_DYNAMIC)
          {
            pRigid1 = static_cast<PxRigidDynamic*>(pairHeader.actors[1]);

            vVelocity1 = ezPxConversionUtils::ToVec3(pRigid1->getLinearVelocity());
          }

          {
            if (pRigid0 && ContactFlags0.IsAnySet(ezOnPhysXContact::RollXReactions | ezOnPhysXContact::RollYReactions | ezOnPhysXContact::RollZReactions))
            {
              const ezVec3 vAngularVel = ezPxConversionUtils::ToVec3(pRigid0->getGlobalPose().rotateInv(pRigid0->getAngularVelocity()));

              // TODO: make threshold tweakable
              if ((ContactFlags0.IsSet(ezOnPhysXContact::RollXReactions) && ezMath::Abs(vAngularVel.x) > 1.0f) ||
                  (ContactFlags0.IsSet(ezOnPhysXContact::RollYReactions) && ezMath::Abs(vAngularVel.y) > 1.0f) ||
                  (ContactFlags0.IsSet(ezOnPhysXContact::RollZReactions) && ezMath::Abs(vAngularVel.z) > 1.0f))
              {
                m_SlidingActors[pRigid0].m_bStillRolling = true;
                m_SlidingActors[pRigid0].m_vPosition = vAvgPos;
                m_SlidingActors[pRigid0].m_vNormal = vAvgNormal;
              }
            }

            if (pRigid1 && ContactFlags1.IsAnySet(ezOnPhysXContact::RollXReactions | ezOnPhysXContact::RollYReactions | ezOnPhysXContact::RollZReactions))
            {
              const ezVec3 vAngularVel = ezPxConversionUtils::ToVec3(pRigid1->getGlobalPose().rotateInv(pRigid1->getAngularVelocity()));

              // TODO: make threshold tweakable
              if ((ContactFlags1.IsSet(ezOnPhysXContact::RollXReactions) && ezMath::Abs(vAngularVel.x) > 1.0f) ||
                  (ContactFlags1.IsSet(ezOnPhysXContact::RollYReactions) && ezMath::Abs(vAngularVel.y) > 1.0f) ||
                  (ContactFlags1.IsSet(ezOnPhysXContact::RollZReactions) && ezMath::Abs(vAngularVel.z) > 1.0f))
              {
                m_SlidingActors[pRigid1].m_bStillRolling = true;
                m_SlidingActors[pRigid1].m_vPosition = vAvgPos;
                m_SlidingActors[pRigid1].m_vNormal = vAvgNormal;
              }
            }
          }

          if (uiNumContactPoints >= 2 && CombinedContactFlags.IsAnySet(ezOnPhysXContact::SlideReactions))
          {
            const ezVec3 vRelativeVelocity = vVelocity1 - vVelocity0;

            if (!vRelativeVelocity.IsZero())
            {
              const ezVec3 vRelativeVelocityDir = vRelativeVelocity.GetNormalized();

              if (ezMath::Abs(vAvgNormal.Dot(vRelativeVelocityDir)) < 0.1f)
              {
                // TODO: make threshold tweakable
                if (vRelativeVelocity.GetLengthSquared() > ezMath::Square(1.0f))
                {
                  if (pRigid0 && ContactFlags0.IsAnySet(ezOnPhysXContact::SlideReactions))
                  {
                    auto& info = m_SlidingActors[pRigid0];

                    if (!info.m_bStillRolling)
                    {
                      info.m_bStillSliding = true;
                      info.m_vPosition = vAvgPos;
                      info.m_vNormal = vAvgNormal;
                    }
                  }

                  if (pRigid1 && ContactFlags1.IsAnySet(ezOnPhysXContact::SlideReactions))
                  {
                    auto& info = m_SlidingActors[pRigid1];

                    if (!info.m_bStillRolling)
                    {
                      info.m_bStillSliding = true;
                      info.m_vPosition = vAvgPos;
                      info.m_vNormal = vAvgNormal;
                    }
                  }

                  //ezLog::Dev("Sliding: {} / {}", ezArgP(pairHeader.actors[0]), ezArgP(pairHeader.actors[1]));
                }
              }
            }
          }
        }

        if (CombinedContactFlags.IsAnySet(ezOnPhysXContact::ImpactReactions))
        {
          const PxContactPairPoint& point = contactPointBuffer[0];

          if (PxMaterial* pMaterial0 = pair.shapes[0]->getMaterialFromInternalFaceIndex(point.internalFaceIndex0))
          {
            if (PxMaterial* pMaterial1 = pair.shapes[1]->getMaterialFromInternalFaceIndex(point.internalFaceIndex1))
            {
              if (ezSurfaceResource* pSurface0 = ezPxUserData::GetSurfaceResource(pMaterial0->userData))
              {
                if (ezSurfaceResource* pSurface1 = ezPxUserData::GetSurfaceResource(pMaterial1->userData))
                {
                  InteractionContact& ic = m_InteractionContacts.ExpandAndGetRef();
                  ic.m_vPosition = vAvgPos;
                  ic.m_vNormal = vAvgNormal;
                  ic.m_vNormal.NormalizeIfNotZero(ezVec3(0, 0, 1));
                  ic.m_fImpulseSqr = fMaxImpactSqr;

                  // if one actor is static or kinematic, prefer to spawn the interaction from its surface definition
                  if (pairHeader.actors[0]->getType() == PxActorType::eRIGID_STATIC ||
                      (pairHeader.actors[0]->getType() == PxActorType::eRIGID_DYNAMIC && static_cast<PxRigidDynamic*>(pairHeader.actors[0])->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC)))
                  {
                    ic.m_pSurface = pSurface0;
                    ic.m_sInteraction = pSurface1->GetDescriptor().m_sOnCollideInteraction;
                  }
                  else
                  {
                    ic.m_pSurface = pSurface1;
                    ic.m_sInteraction = pSurface0->GetDescriptor().m_sOnCollideInteraction;
                  }
                }
              }
            }
          }
        }
      }
    }

    if (bSendContactReport)
    {
      SendContactReport(pairHeader, pairs, nbPairs);
    }
  }

  void SendContactReport(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
  {
    ezMsgCollision msg;
    msg.m_vPosition.SetZero();
    msg.m_vNormal.SetZero();
    msg.m_vImpulse.SetZero();

    float fNumContactPoints = 0.0f;

    ///\todo Not sure whether taking a simple average is the desired behavior here.
    for (ezUInt32 uiPairIndex = 0; uiPairIndex < nbPairs; ++uiPairIndex)
    {
      const PxContactPair& pair = pairs[uiPairIndex];

      PxContactPairPoint contactPointBuffer[16];
      ezUInt32 uiNumContactPoints = pair.extractContacts(contactPointBuffer, 16);

      for (ezUInt32 uiContactPointIndex = 0; uiContactPointIndex < uiNumContactPoints; ++uiContactPointIndex)
      {
        const PxContactPairPoint& point = contactPointBuffer[uiContactPointIndex];

        msg.m_vPosition += ezPxConversionUtils::ToVec3(point.position);
        msg.m_vNormal += ezPxConversionUtils::ToVec3(point.normal);
        msg.m_vImpulse += ezPxConversionUtils::ToVec3(point.impulse);

        fNumContactPoints += 1.0f;
      }
    }

    msg.m_vPosition /= fNumContactPoints;
    msg.m_vNormal.NormalizeIfNotZero();
    msg.m_vImpulse /= fNumContactPoints;

    const PxActor* pActorA = pairHeader.actors[0];
    const PxActor* pActorB = pairHeader.actors[1];

    const ezComponent* pComponentA = ezPxUserData::GetComponent(pActorA->userData);
    const ezComponent* pComponentB = ezPxUserData::GetComponent(pActorB->userData);

    const ezGameObject* pObjectA = nullptr;
    const ezGameObject* pObjectB = nullptr;

    if (pComponentA != nullptr)
    {
      pObjectA = pComponentA->GetOwner();

      msg.m_hObjectA = pObjectA->GetHandle();
      msg.m_hComponentA = pComponentA->GetHandle();
    }

    if (pComponentB != nullptr)
    {
      pObjectB = pComponentB->GetOwner();

      msg.m_hObjectB = pObjectB->GetHandle();
      msg.m_hComponentB = pComponentB->GetHandle();
    }


    if (pObjectA != nullptr)
    {
      pObjectA->PostMessage(msg, ezTime::Zero(), ezObjectMsgQueueType::PostTransform);
    }

    if (pObjectB != nullptr && pObjectB != pObjectA)
    {
      pObjectB->PostMessage(msg, ezTime::Zero(), ezObjectMsgQueueType::PostTransform);
    }
  }

  virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) override
  {
    for (ezUInt32 i = 0; i < count; ++i)
    {
      auto& pair = pairs[i];

      if (pair.flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
      {
        continue;
      }

      const PxActor* pTriggerActor = pair.triggerActor;
      const PxActor* pOtherActor = pair.otherActor;

      const ezPxTriggerComponent* pTriggerComponent = ezPxUserData::GetTriggerComponent(pTriggerActor->userData);
      const ezComponent* pOtherComponent = ezPxUserData::GetComponent(pOtherActor->userData);

      if (pTriggerComponent != nullptr && pOtherComponent != nullptr)
      {
        ezTriggerState::Enum triggerState = (pair.status == PxPairFlag::eNOTIFY_TOUCH_FOUND) ? ezTriggerState::Activated : ezTriggerState::Deactivated;
        pTriggerComponent->PostTriggerMessage(pOtherComponent, triggerState);
      }
    }
  }

  virtual void onAdvance(const PxRigidBody* const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) override {}
};

//////////////////////////////////////////////////////////////////////////

ezPhysXWorldModule::ezPhysXWorldModule(ezWorld* pWorld)
  : ezPhysicsWorldModuleInterface(pWorld)
  , m_pPxScene(nullptr)
  , m_pCharacterManager(nullptr)
  , m_pSimulationEventCallback(nullptr)
  , m_uiNextShapeId(0)
  , m_FreeShapeIds(ezPhysX::GetSingleton()->GetAllocator())
  , m_ScratchMemory(ezPhysX::GetSingleton()->GetAllocator())
  , m_Settings()
{
  m_pSimulateTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "", ezMakeDelegate(&ezPhysXWorldModule::Simulate, this));
  m_pSimulateTask->ConfigureTask("PhysX Simulate", ezTaskNesting::Maybe);

  m_ScratchMemory.SetCountUninitialized(ezMemoryUtils::AlignSize(m_Settings.m_uiScratchMemorySize, 16u * 1024u));
}

ezPhysXWorldModule::~ezPhysXWorldModule() {}

void ezPhysXWorldModule::Initialize()
{
  ezPhysX::GetSingleton()->LoadCollisionFilters();

  m_AccumulatedTimeSinceUpdate.SetZero();

  m_pSimulationEventCallback = EZ_DEFAULT_NEW(ezPxSimulationEventCallback);

  {
    PxSceneDesc desc = PxSceneDesc(PxTolerancesScale());
    desc.setToDefault(PxTolerancesScale());

    desc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
    desc.flags |= PxSceneFlag::eEXCLUDE_KINEMATICS_FROM_ACTIVE_ACTORS;
    desc.kineKineFilteringMode = PxPairFilteringMode::eKEEP;
    desc.flags |= PxSceneFlag::eADAPTIVE_FORCE;
    desc.flags |= PxSceneFlag::eREQUIRE_RW_LOCK;
    desc.flags |= PxSceneFlag::eENABLE_CCD;

    desc.gravity = ezPxConversionUtils::ToVec3(m_Settings.m_vObjectGravity);

    desc.cpuDispatcher = &s_CpuDispatcher;
    desc.filterShader = &ezPxFilterShader;
    desc.simulationEventCallback = m_pSimulationEventCallback;

    EZ_ASSERT_DEV(desc.isValid(), "PhysX scene description is invalid");
    m_pPxScene = ezPhysX::GetSingleton()->GetPhysXAPI()->createScene(desc);
    EZ_ASSERT_ALWAYS(m_pPxScene != nullptr, "Creating the PhysX scene failed");
  }

  m_pCharacterManager = PxCreateControllerManager(*m_pPxScene);

  {
    auto startSimDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezPhysXWorldModule::StartSimulation, this);
    startSimDesc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    startSimDesc.m_bOnlyUpdateWhenSimulating = true;
    // Start physics simulation as late as possible in the first synchronous phase
    // so all kinematic objects have a chance to update their transform before.
    startSimDesc.m_fPriority = -100000.0f;

    RegisterUpdateFunction(startSimDesc);
  }

  {
    auto fetchResultsDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezPhysXWorldModule::FetchResults, this);
    fetchResultsDesc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    fetchResultsDesc.m_bOnlyUpdateWhenSimulating = true;
    // Fetch results as early as possible after async phase.
    fetchResultsDesc.m_fPriority = 100000.0f;

    RegisterUpdateFunction(fetchResultsDesc);
  }
}

void ezPhysXWorldModule::Deinitialize()
{
  m_pCharacterManager->release();
  m_pCharacterManager = nullptr;

  m_pPxScene->release();
  m_pPxScene = nullptr;

  EZ_DEFAULT_DELETE(m_pSimulationEventCallback);
}

void ezPhysXWorldModule::OnSimulationStarted()
{
  ezPhysX* pPhysX = ezPhysX::GetSingleton();

  pPhysX->LoadCollisionFilters();

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  pPhysX->StartupVDB();
#endif
}

ezUInt32 ezPhysXWorldModule::CreateShapeId()
{
  if (!m_FreeShapeIds.IsEmpty())
  {
    ezUInt32 uiShapeId = m_FreeShapeIds.PeekBack();
    m_FreeShapeIds.PopBack();

    return uiShapeId;
  }

  return m_uiNextShapeId++;
}

void ezPhysXWorldModule::DeleteShapeId(ezUInt32& uiShapeId)
{
  if (uiShapeId == ezInvalidIndex)
    return;

  m_FreeShapeIds.PushBack(uiShapeId);

  uiShapeId = ezInvalidIndex;
}

ezUInt32 ezPhysXWorldModule::AllocateUserData(ezPxUserData*& out_pUserData)
{
  if (!m_FreeUserData.IsEmpty())
  {
    ezUInt32 uiIndex = m_FreeUserData.PeekBack();
    m_FreeUserData.PopBack();

    out_pUserData = &m_AllocatedUserData[uiIndex];
    return uiIndex;
  }

  out_pUserData = &m_AllocatedUserData.ExpandAndGetRef();
  return m_AllocatedUserData.GetCount() - 1;
}

void ezPhysXWorldModule::DeallocateUserData(ezUInt32& uiUserDataIndex)
{
  if (uiUserDataIndex == ezInvalidIndex)
    return;

  m_AllocatedUserData[uiUserDataIndex].Invalidate();

  m_FreeUserDataAfterSimulationStep.PushBack(uiUserDataIndex);

  uiUserDataIndex = ezInvalidIndex;
}

void ezPhysXWorldModule::SetGravity(const ezVec3& objectGravity, const ezVec3& characterGravity)
{
  m_Settings.m_vObjectGravity = objectGravity;
  m_Settings.m_vCharacterGravity = characterGravity;

  if (m_pPxScene)
  {
    EZ_PX_WRITE_LOCK(*m_pPxScene);

    m_pPxScene->setGravity(ezPxConversionUtils::ToVec3(m_Settings.m_vObjectGravity));
  }
}

bool ezPhysXWorldModule::Raycast(ezPhysicsCastResult& out_Result, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection /*= ezPhysicsHitCollection::Closest*/) const
{
  if (fDistance <= 0.001f || vDir.IsZero())
    return false;

  PxQueryFilterData filterData;
  filterData.data = ezPhysX::CreateFilterData(params.m_uiCollisionLayer, params.m_uiIgnoreShapeId);
  filterData.flags = PxQueryFlag::ePREFILTER;

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Static))
  {
    filterData.flags |= PxQueryFlag::eSTATIC;
  }

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Dynamic))
  {
    filterData.flags |= PxQueryFlag::eDYNAMIC;
  }

  if (collection == ezPhysicsHitCollection::Any)
  {
    filterData.flags |= PxQueryFlag::eANY_HIT;
  }

  ezPxRaycastCallback closestHit;
  ezPxQueryFilter queryFilter;

  EZ_PX_READ_LOCK(*m_pPxScene);

  if (m_pPxScene->raycast(ezPxConversionUtils::ToVec3(vStart), ezPxConversionUtils::ToVec3(vDir), fDistance, closestHit, PxHitFlag::eDEFAULT, filterData, &queryFilter))
  {
    FillHitResult(closestHit.block, out_Result);

    return true;
  }

  return false;
}

bool ezPhysXWorldModule::RaycastAll(ezPhysicsCastResultArray& out_Results, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params) const
{
  if (fDistance <= 0.001f || vDir.IsZero())
    return false;

  PxQueryFilterData filterData;
  filterData.data = ezPhysX::CreateFilterData(params.m_uiCollisionLayer, params.m_uiIgnoreShapeId);

  // PxQueryFlag::eNO_BLOCK : All hits are reported as touching. Overrides eBLOCK returned from user filters with eTOUCH.
  filterData.flags = PxQueryFlag::ePREFILTER | PxQueryFlag::eNO_BLOCK;

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Static))
  {
    filterData.flags |= PxQueryFlag::eSTATIC;
  }

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Dynamic))
  {
    filterData.flags |= PxQueryFlag::eDYNAMIC;
  }

  g_RaycastHits.SetCountUninitialized(256);
  PxRaycastBuffer allHits(g_RaycastHits.GetData(), g_RaycastHits.GetCount());

  ezPxQueryFilter queryFilter;

  EZ_PX_READ_LOCK(*m_pPxScene);

  if (m_pPxScene->raycast(ezPxConversionUtils::ToVec3(vStart), ezPxConversionUtils::ToVec3(vDir), fDistance, allHits,
        PxHitFlag::eDEFAULT | PxHitFlag::eMESH_MULTIPLE | PxHitFlag::eMESH_BOTH_SIDES, filterData, &queryFilter))
  {
    out_Results.m_Results.SetCount(allHits.nbTouches);

    for (ezUInt32 i = 0; i < allHits.nbTouches; ++i)
    {
      FillHitResult(allHits.touches[i], out_Results.m_Results[i]);
    }

    return true;
  }

  return false;
}

void* ezPhysXWorldModule::CreateRagdoll(const ezSkeletonResourceDescriptor& skeleton, const ezTransform& rootTransform0,
  const ezAnimationPose& initPose)
{
  const float fScale = rootTransform0.m_vScale.x;
  const ezTransform rootTransform(rootTransform0.m_vPosition, rootTransform0.m_qRotation);

  const PxMaterial* pPxMaterial = ezPhysX::GetSingleton()->GetDefaultMaterial();
  const PxFilterData filter = ezPhysX::CreateFilterData(/*m_uiCollisionLayer*/ 0);

  physx::PxArticulation* pArt = m_pPxScene->getPhysics().createArticulation();

  //if (false)
  //{
  //  ezTransform tRoot;
  //  tRoot.SetIdentity();
  //  tRoot.m_vPosition.z = 5.0f;
  //  PxArticulationLink* pRootLink = pArt->createLink(nullptr, ezPxConversionUtils::ToTransform(tRoot));
  //  // pRootLink->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);

  //  {
  //    // PxBoxGeometry shape(1, 0.1, 0.5);
  //    PxSphereGeometry shape(0.5f);
  //    PxShape* pShape = PxRigidActorExt::createExclusiveShape(*pRootLink, shape, *pPxMaterial);
  //    PxRigidBodyExt::updateMassAndInertia(*pRootLink, 1.0f);
  //    pShape->setSimulationFilterData(filter);
  //    pShape->setQueryFilterData(filter);
  //  }

  //  ezTransform tChild;
  //  tChild.SetIdentity();
  //  tChild.m_vPosition.z = 5.0f;
  //  tChild.m_vPosition.x = 2.0f;
  //  PxArticulationLink* pChildLink = pArt->createLink(pRootLink, ezPxConversionUtils::ToTransform(tChild));
  //  pChildLink->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);

  //  ezTransform jointParentPose;
  //  jointParentPose.SetIdentity();
  //  pChildLink->getInboundJoint()->setParentPose(ezPxConversionUtils::ToTransform(jointParentPose));

  //  ezTransform jointChildPose;
  //  jointChildPose.SetIdentity();
  //  jointChildPose.m_vPosition.x = -2.0f;
  //  // jointChildPose.SetLocalTransform(tChild, ezTransform::IdentityTransform());
  //  pChildLink->getInboundJoint()->setChildPose(ezPxConversionUtils::ToTransform(jointChildPose));

  //  {
  //    PxBoxGeometry shape(0.8, 0.1, 0.4);
  //    PxShape* pShape = PxRigidActorExt::createExclusiveShape(*pChildLink, shape, *pPxMaterial);
  //    PxRigidBodyExt::updateMassAndInertia(*pChildLink, 1.0f);
  //    pShape->setSimulationFilterData(filter);
  //    pShape->setQueryFilterData(filter);
  //  }
  //}
  //else
  {
    ezMap<ezUInt16, PxArticulationLink*> links;

    //{
    //  PxArticulationLink* pRootLink = pArt->createLink(nullptr, ezPxConversionUtils::ToTransform(rootTransform));
    //  // pRootLink->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
    //  links[ezInvalidJointIndex] = pRootLink;
    //  pRootLink->setName("Ragdoll");
    //}

    for (const ezSkeletonResourceGeometry& geom : skeleton.m_Geometry)
    {
      const ezSkeletonJoint& joint = skeleton.m_Skeleton.GetJointByIndex(geom.m_uiAttachedToJoint);

      PxArticulationLink* pThisLink = nullptr;

      bool bExisted = false;
      auto itLink = links.FindOrAdd(geom.m_uiAttachedToJoint, &bExisted);
      if (!bExisted)
      {
        ezUInt16 uiParentJoint = joint.GetParentIndex();
        PxArticulationLink* pParentLink = links.GetValueOrDefault(uiParentJoint, nullptr);

        if (links.GetCount() > 1)
        {
          while (pParentLink == nullptr)
          {
            uiParentJoint = skeleton.m_Skeleton.GetJointByIndex(uiParentJoint).GetParentIndex();
            pParentLink = links.GetValueOrDefault(uiParentJoint, nullptr);
          }
        }
        else
        {
          uiParentJoint = ezInvalidJointIndex;
        }

        ezTransform parentTransformAbs;
        ezTransform thisTransformAbs;

        // compute link transforms
        {
          if (uiParentJoint == ezInvalidJointIndex)
            parentTransformAbs = rootTransform;
          else
          {
            ezTransform poseTransform;
            poseTransform.SetFromMat4(initPose.GetTransform(uiParentJoint));

            parentTransformAbs = rootTransform * ezTransform(poseTransform.m_vPosition * fScale, poseTransform.m_qRotation);
          }

          {
            ezTransform poseTransform;
            poseTransform.SetFromMat4(initPose.GetTransform(geom.m_uiAttachedToJoint));

            thisTransformAbs = rootTransform * ezTransform(poseTransform.m_vPosition * fScale, poseTransform.m_qRotation);
          }
        }

        pThisLink = pArt->createLink(pParentLink, ezPxConversionUtils::ToTransform(thisTransformAbs));
        itLink.Value() = pThisLink;

        pThisLink->setName(joint.GetName().GetData());
        //pThisLink->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);

        if (PxArticulationJointBase* pJoint = pThisLink->getInboundJoint())
        {
          pJoint->setChildPose(ezPxConversionUtils::ToTransform(ezTransform::IdentityTransform()));

          ezTransform parentJointTransform;
          parentJointTransform.SetLocalTransform(parentTransformAbs, thisTransformAbs);
          pJoint->setParentPose(ezPxConversionUtils::ToTransform(parentJointTransform));
          //TODO: Commented out after PhysX4 upgrade.
          //pJoint->setTwistLimitEnabled(true);
          //pJoint->setSwingLimitEnabled(true);
        }

        if (links.GetCount() == 1)
        {
          links[ezInvalidJointIndex] = pThisLink;
        }
      }

      pThisLink = itLink.Value();
      PxShape* pShape = nullptr;

      switch (geom.m_Type)
      {
        case ezSkeletonJointGeometryType::Box:
        {
          PxBoxGeometry shape(fScale * geom.m_Transform.m_vScale.x, fScale * geom.m_Transform.m_vScale.y,
            fScale * geom.m_Transform.m_vScale.z);
          pShape = PxRigidActorExt::createExclusiveShape(*itLink.Value(), shape, *pPxMaterial);
          break;
        }

        case ezSkeletonJointGeometryType::Sphere:
        {
          PxSphereGeometry shape(fScale * geom.m_Transform.m_vScale.z);
          pShape = PxRigidActorExt::createExclusiveShape(*itLink.Value(), shape, *pPxMaterial);
          break;
        }

        case ezSkeletonJointGeometryType::Capsule:
        {
          PxCapsuleGeometry shape(fScale * geom.m_Transform.m_vScale.z, fScale * geom.m_Transform.m_vScale.x);
          pShape = PxRigidActorExt::createExclusiveShape(*itLink.Value(), shape, *pPxMaterial);
          break;
        }
      }

      // create shape
      {
        PxRigidBodyExt::updateMassAndInertia(*itLink.Value(), 1.0f);

        pShape->setSimulationFilterData(filter);
        pShape->setQueryFilterData(filter);
      }
    }
  }

  EZ_PX_WRITE_LOCK(*m_pPxScene);
  m_pPxScene->addArticulation(*pArt);

  return pArt;
}

bool ezPhysXWorldModule::SweepTestSphere(ezPhysicsCastResult& out_Result, float fSphereRadius, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const
{
  PxSphereGeometry sphere;
  sphere.radius = fSphereRadius;

  PxTransform transform = ezPxConversionUtils::ToTransform(vStart, ezQuat::IdentityQuaternion());

  return SweepTest(out_Result, sphere, transform, vDir, fDistance, params, collection);
}

bool ezPhysXWorldModule::SweepTestBox(ezPhysicsCastResult& out_Result, ezVec3 vBoxExtends, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const
{
  PxBoxGeometry box;
  box.halfExtents = ezPxConversionUtils::ToVec3(vBoxExtends * 0.5f);

  return SweepTest(out_Result, box, ezPxConversionUtils::ToTransform(transform), vDir, fDistance, params, collection);
}

bool ezPhysXWorldModule::SweepTestCapsule(ezPhysicsCastResult& out_Result, float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const
{
  PxCapsuleGeometry capsule;
  capsule.radius = fCapsuleRadius;
  capsule.halfHeight = fCapsuleHeight * 0.5f;
  EZ_ASSERT_DEBUG(capsule.isValid(), "Invalid capsule parameter. Radius = {0}, Height = {1}", ezArgF(fCapsuleRadius, 2),
    ezArgF(fCapsuleHeight, 2));

  ezQuat qFixRot;
  qFixRot.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90.0f));

  ezQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qFixRot * qRot;

  return SweepTest(out_Result, capsule, ezPxConversionUtils::ToTransform(transform.m_vPosition, qRot), vDir, fDistance, params, collection);
}

bool ezPhysXWorldModule::SweepTest(ezPhysicsCastResult& out_Result, const physx::PxGeometry& geometry, const physx::PxTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const
{
  PxQueryFilterData filterData;
  filterData.data = ezPhysX::CreateFilterData(params.m_uiCollisionLayer, params.m_uiIgnoreShapeId);

  filterData.flags = PxQueryFlag::ePREFILTER;

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Static))
  {
    filterData.flags |= PxQueryFlag::eSTATIC;
  }

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Dynamic))
  {
    filterData.flags |= PxQueryFlag::eDYNAMIC;
  }

  if (collection == ezPhysicsHitCollection::Any)
  {
    filterData.flags |= PxQueryFlag::eANY_HIT;
  }

  ezPxSweepCallback closestHit;
  ezPxQueryFilter queryFilter;

  EZ_PX_READ_LOCK(*m_pPxScene);

  if (m_pPxScene->sweep(geometry, transform, ezPxConversionUtils::ToVec3(vDir), fDistance, closestHit, PxHitFlag::eDEFAULT, filterData,
        &queryFilter))
  {
    FillHitResult(closestHit.block, out_Result);

    return true;
  }

  return false;
}

bool ezPhysXWorldModule::OverlapTestSphere(float fSphereRadius, const ezVec3& vPosition, const ezPhysicsQueryParameters& params) const
{
  PxSphereGeometry sphere;
  sphere.radius = fSphereRadius;

  PxTransform transform = ezPxConversionUtils::ToTransform(vPosition, ezQuat::IdentityQuaternion());

  return OverlapTest(sphere, transform, params);
}

bool ezPhysXWorldModule::OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezPhysicsQueryParameters& params) const
{
  PxCapsuleGeometry capsule;
  capsule.radius = fCapsuleRadius;
  capsule.halfHeight = fCapsuleHeight * 0.5f;
  EZ_ASSERT_DEBUG(capsule.isValid(), "Invalid capsule parameter. Radius = {0}, Height = {1}", ezArgF(fCapsuleRadius, 2),
    ezArgF(fCapsuleHeight, 2));

  ezQuat qFixRot;
  qFixRot.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90.0f));

  ezQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qFixRot * qRot;

  return OverlapTest(capsule, ezPxConversionUtils::ToTransform(transform.m_vPosition, qRot), params);
}

bool ezPhysXWorldModule::OverlapTest(const physx::PxGeometry& geometry, const physx::PxTransform& transform, const ezPhysicsQueryParameters& params) const
{
  PxQueryFilterData filterData;
  filterData.data = ezPhysX::CreateFilterData(params.m_uiCollisionLayer, params.m_uiIgnoreShapeId);

  filterData.flags = PxQueryFlag::ePREFILTER | PxQueryFlag::eANY_HIT;

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Static))
  {
    filterData.flags |= PxQueryFlag::eSTATIC;
  }

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Dynamic))
  {
    filterData.flags |= PxQueryFlag::eDYNAMIC;
  }

  ezPxOverlapCallback closestHit;
  ezPxQueryFilter queryFilter;

  EZ_PX_READ_LOCK(*m_pPxScene);

  return m_pPxScene->overlap(geometry, transform, closestHit, filterData, &queryFilter);
}

void ezPhysXWorldModule::QueryShapesInSphere(ezPhysicsOverlapResultArray& out_Results, float fSphereRadius, const ezVec3& vPosition, const ezPhysicsQueryParameters& params) const
{
  EZ_PROFILE_SCOPE("QueryShapesInSphere");

  out_Results.m_Results.Clear();

  PxQueryFilterData filterData;
  filterData.data = ezPhysX::CreateFilterData(params.m_uiCollisionLayer, params.m_uiIgnoreShapeId);

  // PxQueryFlag::eNO_BLOCK : All hits are reported as touching. Overrides eBLOCK returned from user filters with eTOUCH.
  filterData.flags = PxQueryFlag::ePREFILTER | PxQueryFlag::eNO_BLOCK;

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Static))
  {
    filterData.flags |= PxQueryFlag::eSTATIC;
  }

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Dynamic))
  {
    filterData.flags |= PxQueryFlag::eDYNAMIC;
  }

  ezPxQueryFilter queryFilter;

  PxSphereGeometry sphere;
  sphere.radius = fSphereRadius;

  PxTransform transform = ezPxConversionUtils::ToTransform(vPosition, ezQuat::IdentityQuaternion());

  g_OverlapHits.SetCountUninitialized(256);
  PxOverlapBuffer overlapHitsBuffer(g_OverlapHits.GetData(), g_OverlapHits.GetCount());

  EZ_PX_READ_LOCK(*m_pPxScene);

  m_pPxScene->overlap(sphere, transform, overlapHitsBuffer, filterData, &queryFilter);

  out_Results.m_Results.Reserve(overlapHitsBuffer.nbTouches);

  for (ezUInt32 i = 0; i < overlapHitsBuffer.nbTouches; ++i)
  {
    auto& overlapHit = overlapHitsBuffer.touches[i];
    auto& overlapResult = out_Results.m_Results.ExpandAndGetRef();

    if (ezComponent* pShapeComponent = ezPxUserData::GetComponent(overlapHit.shape->userData))
    {
      overlapResult.m_hShapeObject = pShapeComponent->GetOwner()->GetHandle();
      overlapResult.m_uiShapeId = overlapHit.shape->getQueryFilterData().word2;
    }

    if (ezComponent* pActorComponent = ezPxUserData::GetComponent(overlapHit.actor->userData))
    {
      overlapResult.m_hActorObject = pActorComponent->GetOwner()->GetHandle();
    }
  }
}


void ezPhysXWorldModule::AddStaticCollisionBox(ezGameObject* pObject, ezVec3 boxSize)
{
  ezPxStaticActorComponent* pActor = nullptr;
  ezPxStaticActorComponent::CreateComponent(pObject, pActor);

  ezPxShapeBoxComponent* pBox;
  ezPxShapeBoxComponent::CreateComponent(pObject, pBox);
  pBox->SetExtents(boxSize);
}

void ezPhysXWorldModule::FreeUserDataAfterSimulationStep()
{
  m_FreeUserData.PushBackRange(m_FreeUserDataAfterSimulationStep);
  m_FreeUserDataAfterSimulationStep.Clear();
}

void ezPhysXWorldModule::StartSimulation(const ezWorldModule::UpdateContext& context)
{
  ezPxDynamicActorComponentManager* pDynamicActorManager = GetWorld()->GetComponentManager<ezPxDynamicActorComponentManager>();
  ezPxTriggerComponentManager* pTriggerManager = GetWorld()->GetComponentManager<ezPxTriggerComponentManager>();

  EZ_PX_WRITE_LOCK(*m_pPxScene);

  if (ezPxSettingsComponentManager* pSettingsManager = GetWorld()->GetComponentManager<ezPxSettingsComponentManager>())
  {
    ezPxSettingsComponent* pSettings = pSettingsManager->GetSingletonComponent();
    if (pSettings != nullptr && pSettings->IsModified())
    {
      SetGravity(pSettings->GetObjectGravity(), pSettings->GetCharacterGravity());

      if (pDynamicActorManager != nullptr && pSettings->IsModified(EZ_BIT(2))) // max de-penetration velocity
      {
        pDynamicActorManager->UpdateMaxDepenetrationVelocity(pSettings->GetMaxDepenetrationVelocity());
      }

      m_Settings = pSettings->GetSettings();
      m_AccumulatedTimeSinceUpdate.SetZero();

      m_ScratchMemory.SetCountUninitialized(ezMemoryUtils::AlignSize(m_Settings.m_uiScratchMemorySize, 16u * 1024u));

      pSettings->ResetModified();
    }
  }

  if (pDynamicActorManager != nullptr)
  {
    pDynamicActorManager->UpdateKinematicActors();
  }

  if (pTriggerManager != nullptr)
  {
    pTriggerManager->UpdateKinematicActors();
  }

  m_SimulateTaskGroupId = ezTaskSystem::StartSingleTask(m_pSimulateTask, ezTaskPriority::EarlyThisFrame);
}

void ezPhysXWorldModule::FetchResults(const ezWorldModule::UpdateContext& context)
{
  {
    EZ_PROFILE_SCOPE("Wait for Simulate Task");
    ezTaskSystem::WaitForGroup(m_SimulateTaskGroupId);
  }

  // Nothing to fetch if no simulation step was executed
  if (!m_bSimulationStepExecuted)
    return;

  if (ezPxDynamicActorComponentManager* pDynamicActorManager = GetWorld()->GetComponentManager<ezPxDynamicActorComponentManager>())
  {
    EZ_PX_READ_LOCK(*m_pPxScene);

    PxU32 numActiveActors = 0;
    PxActor** pActiveActors = m_pPxScene->getActiveActors(numActiveActors);

    if (numActiveActors > 0)
    {
      pDynamicActorManager->UpdateDynamicActors(ezMakeArrayPtr(pActiveActors, numActiveActors));
    }
  }

  {
    // TODO: sort by impulse, cluster by position, only execute the first N contacts, prevent duplicate spawns at same location within short time

    for (const auto& ic : m_pSimulationEventCallback->m_InteractionContacts)
    {
      ic.m_pSurface->InteractWithSurface(m_pWorld, ezGameObjectHandle(), ic.m_vPosition, ic.m_vNormal, -ic.m_vNormal, ic.m_sInteraction, nullptr, ic.m_fImpulseSqr);
    }

    m_pSimulationEventCallback->m_InteractionContacts.Clear();
  }

  {
    for (auto& itSlide : m_pSimulationEventCallback->m_SlidingActors)
    {
      //ic.m_pSurface->InteractWithSurface(m_pWorld, ezGameObjectHandle(), ic.m_vPosition, ic.m_vNormal, -ic.m_vNormal, ic.m_sInteraction, nullptr, ic.m_fImpulseSqr);

      auto& slideInfo = itSlide.Value();

      if (slideInfo.m_bStillRolling)
      {
        if (slideInfo.m_bStartedRolling == false)
        {
          slideInfo.m_bStartedRolling = true;
          //ezLog::Dev("Started Rolling");

          // TODO: make roll reaction configurable
          ezPrefabResourceHandle hPrefab = ezResourceManager::LoadResource<ezPrefabResource>("{ 4d306cc5-c1e6-4ec9-a04d-b804e3755210 }");
          ezResourceLock<ezPrefabResource> pPrefab(hPrefab, ezResourceAcquireMode::AllowLoadingFallback_NeverFail);
          if (pPrefab.GetAcquireResult() == ezResourceAcquireResult::Final)
          {
            ezHybridArray<ezGameObject*, 8> created;
            pPrefab->InstantiatePrefab(*m_pWorld, ezTransform(slideInfo.m_vPosition), ezGameObjectHandle(), &created, nullptr, nullptr, true);
            slideInfo.m_hRollPrefab = created[0]->GetHandle();
          }
        }
        else
        {
          ezGameObject* pObject;
          if (m_pWorld->TryGetObject(slideInfo.m_hRollPrefab, pObject))
          {
            pObject->SetGlobalPosition(slideInfo.m_vPosition);
          }
        }

        slideInfo.m_bStillRolling = false;
      }
      else
      {
        if (slideInfo.m_bStartedRolling == true)
        {
          m_pWorld->DeleteObjectDelayed(slideInfo.m_hRollPrefab);
          slideInfo.m_hRollPrefab.Invalidate();

          slideInfo.m_bStartedRolling = false;
          //ezLog::Dev("Stopped Rolling");
        }
      }

      if (slideInfo.m_bStillSliding)
      {
        if (slideInfo.m_bStartedSliding == false)
        {
          slideInfo.m_bStartedSliding = true;
          //ezLog::Dev("Started Sliding");

          // TODO: make slide reaction configurable
          ezPrefabResourceHandle hPrefab = ezResourceManager::LoadResource<ezPrefabResource>("{ c2d8d66d-b123-4cf1-b123-4d015fc69fb0 }");
          ezResourceLock<ezPrefabResource> pPrefab(hPrefab, ezResourceAcquireMode::AllowLoadingFallback_NeverFail);
          if (pPrefab.GetAcquireResult() == ezResourceAcquireResult::Final)
          {
            ezHybridArray<ezGameObject*, 8> created;
            pPrefab->InstantiatePrefab(*m_pWorld, ezTransform(slideInfo.m_vPosition), ezGameObjectHandle(), &created, nullptr, nullptr, true);
            slideInfo.m_hSlidePrefab = created[0]->GetHandle();
          }
        }
        else
        {
          ezGameObject* pObject;
          if (m_pWorld->TryGetObject(slideInfo.m_hSlidePrefab, pObject))
          {
            pObject->SetGlobalPosition(slideInfo.m_vPosition);
          }
        }

        slideInfo.m_bStillSliding = false;
      }
      else
      {
        if (slideInfo.m_bStartedSliding == true)
        {
          m_pWorld->DeleteObjectDelayed(slideInfo.m_hSlidePrefab);
          slideInfo.m_hSlidePrefab.Invalidate();

          slideInfo.m_bStartedSliding = false;
          //ezLog::Dev("Stopped Sliding");
        }
      }
    }
  }

  HandleBrokenConstraints();

  FreeUserDataAfterSimulationStep();
}

void ezPhysXWorldModule::HandleBrokenConstraints()
{
  for (auto pConstraint : m_pSimulationEventCallback->m_BrokenConstraints)
  {
    auto it = m_BreakableJoints.Find(pConstraint);
    if (it.IsValid())
    {
      ezPxJointComponent* pJoint = nullptr;

      if (m_pWorld->TryGetComponent(it.Value(), pJoint))
      {
        ezMsgPhysicsJointBroke msg;
        msg.m_hJointObject = pJoint->GetOwner()->GetHandle();

        pJoint->GetOwner()->PostEventMessage(msg, pJoint, ezTime::Zero());
      }

      // it can't break twice
      m_BreakableJoints.Remove(it);
    }
  }

  m_pSimulationEventCallback->m_BrokenConstraints.Clear();
}

void ezPhysXWorldModule::Simulate()
{
  m_bSimulationStepExecuted = false;
  const ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();

  if (m_Settings.m_SteppingMode == ezPxSteppingMode::Variable)
  {
    SimulateStep(tDiff);
  }
  else if (m_Settings.m_SteppingMode == ezPxSteppingMode::Fixed)
  {
    const ezTime tFixedStep = ezTime::Seconds(1.0 / m_Settings.m_fFixedFrameRate);

    m_AccumulatedTimeSinceUpdate += tDiff;
    ezUInt32 uiNumSubSteps = 0;

    while (m_AccumulatedTimeSinceUpdate >= tFixedStep && uiNumSubSteps < m_Settings.m_uiMaxSubSteps)
    {
      SimulateStep(tFixedStep);

      m_AccumulatedTimeSinceUpdate -= tFixedStep;
      ++uiNumSubSteps;
    }
  }
  else if (m_Settings.m_SteppingMode == ezPxSteppingMode::SemiFixed)
  {
    m_AccumulatedTimeSinceUpdate += tDiff;
    ezTime tFixedStep = ezTime::Seconds(1.0 / m_Settings.m_fFixedFrameRate);
    ezTime tMinStep = tFixedStep * 0.25;

    if (tFixedStep * m_Settings.m_uiMaxSubSteps < m_AccumulatedTimeSinceUpdate)
    {
      tFixedStep = m_AccumulatedTimeSinceUpdate / (double)m_Settings.m_uiMaxSubSteps;
    }

    while (m_AccumulatedTimeSinceUpdate > tMinStep)
    {
      ezTime tDeltaTime = ezMath::Min(tFixedStep, m_AccumulatedTimeSinceUpdate);

      SimulateStep(tDeltaTime);

      m_AccumulatedTimeSinceUpdate -= tDeltaTime;
    }
  }
  else
  {
    EZ_REPORT_FAILURE("Invalid stepping mode");
  }

  // not sure where exactly this needs to go
  // m_pCharacterManager->computeInteractions((float)tDiff.GetSeconds());
}

void ezPhysXWorldModule::SimulateStep(ezTime deltaTime)
{
  {
    EZ_PX_WRITE_LOCK(*m_pPxScene);

    EZ_PROFILE_SCOPE("Simulate");
    m_pPxScene->simulate(deltaTime.AsFloatInSeconds(), nullptr, m_ScratchMemory.GetData(), m_ScratchMemory.GetCount());
  }

  {
    EZ_PROFILE_SCOPE("FetchResult");

    // Help executing tasks while we wait for the simulation to finish
    ezTaskSystem::WaitForCondition([&] { return m_pPxScene->checkResults(false); });

    EZ_PX_WRITE_LOCK(*m_pPxScene);

    int numFetch = 0;
    while (!m_pPxScene->fetchResults(false))
    {
      ++numFetch;
    }

    EZ_ASSERT_DEBUG(numFetch == 0, "m_pPxScene->fetchResults should have succeeded right away");
  }

  m_bSimulationStepExecuted = true;
}



EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_WorldModule_Implementation_PhysXWorldModule);
