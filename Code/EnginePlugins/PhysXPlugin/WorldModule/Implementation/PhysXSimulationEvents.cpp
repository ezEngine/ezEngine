#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/Messages/CollisionMessage.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <PhysXPlugin/Components/PxTriggerComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>

void ezPxSimulationEventCallback::onConstraintBreak(PxConstraintInfo* constraints, PxU32 count)
{
  for (ezUInt32 i = 0; i < count; ++i)
  {
    m_BrokenConstraints.PushBack(constraints[i].constraint);
  }
}

void ezPxSimulationEventCallback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
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

    if (CombinedContactFlags.IsAnySet(ezOnPhysXContact::ImpactReactions | ezOnPhysXContact::SlideReactions | ezOnPhysXContact::RollXReactions | ezOnPhysXContact::RollYReactions | ezOnPhysXContact::RollZReactions))
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
      vAvgNormal.NormalizeIfNotZero(ezVec3::ZeroVector()).IgnoreResult();

      if (CombinedContactFlags.IsAnySet(ezOnPhysXContact::SlideReactions | ezOnPhysXContact::RollXReactions | ezOnPhysXContact::RollYReactions | ezOnPhysXContact::RollZReactions) && !pair.flags.isSet(PxContactPairFlag::eACTOR_PAIR_HAS_FIRST_TOUCH))
      {
        ezVec3 vVelocity0(0.0f);
        ezVec3 vVelocity1(0.0f);
        PxRigidDynamic* pRigid0 = nullptr;
        PxRigidDynamic* pRigid1 = nullptr;

        if (pairHeader.actors[0]->getType() == PxActorType::eRIGID_DYNAMIC)
        {
          pRigid0 = static_cast<PxRigidDynamic*>(pairHeader.actors[0]);

          vVelocity0 = ezPxConversionUtils::ToVec3(pRigid0->getLinearVelocity());

          if (!vVelocity0.IsValid())
            vVelocity0.SetZero();
        }

        if (pairHeader.actors[1]->getType() == PxActorType::eRIGID_DYNAMIC)
        {
          pRigid1 = static_cast<PxRigidDynamic*>(pairHeader.actors[1]);

          vVelocity1 = ezPxConversionUtils::ToVec3(pRigid1->getLinearVelocity());

          if (!vVelocity1.IsValid())
            vVelocity1.SetZero();
        }

        {
          if (pRigid0 && ContactFlags0.IsAnySet(ezOnPhysXContact::RollXReactions | ezOnPhysXContact::RollYReactions | ezOnPhysXContact::RollZReactions))
          {
            const ezVec3 vAngularVel = ezPxConversionUtils::ToVec3(pRigid0->getGlobalPose().rotateInv(pRigid0->getAngularVelocity()));

            // TODO: make threshold tweakable
            if ((ContactFlags0.IsSet(ezOnPhysXContact::RollXReactions) && ezMath::Abs(vAngularVel.x) > 1.0f) || (ContactFlags0.IsSet(ezOnPhysXContact::RollYReactions) && ezMath::Abs(vAngularVel.y) > 1.0f) ||
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
            if ((ContactFlags1.IsSet(ezOnPhysXContact::RollXReactions) && ezMath::Abs(vAngularVel.x) > 1.0f) || (ContactFlags1.IsSet(ezOnPhysXContact::RollYReactions) && ezMath::Abs(vAngularVel.y) > 1.0f) ||
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

          if (!vRelativeVelocity.IsZero(0.0001f))
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

                // ezLog::Dev("Sliding: {} / {}", ezArgP(pairHeader.actors[0]), ezArgP(pairHeader.actors[1]));
              }
            }
          }
        }
      }

      if (CombinedContactFlags.IsAnySet(ezOnPhysXContact::ImpactReactions) && m_InteractionContacts.GetCount() < 8)
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
                ic.m_vNormal.NormalizeIfNotZero(ezVec3(0, 0, 1)).IgnoreResult();
                ic.m_fImpulseSqr = fMaxImpactSqr;

                // if one actor is static or kinematic, prefer to spawn the interaction from its surface definition
                if (pairHeader.actors[0]->getType() == PxActorType::eRIGID_STATIC || (pairHeader.actors[0]->getType() == PxActorType::eRIGID_DYNAMIC && static_cast<PxRigidDynamic*>(pairHeader.actors[0])->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC)))
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

void ezPxSimulationEventCallback::SendContactReport(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
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
  msg.m_vNormal.NormalizeIfNotZero().IgnoreResult();
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

void ezPxSimulationEventCallback::onTrigger(PxTriggerPair* pairs, PxU32 count)
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
      auto& te = m_TriggerEvents.ExpandAndGetRef();

      te.m_TriggerState = (pair.status == PxPairFlag::eNOTIFY_TOUCH_FOUND) ? ezTriggerState::Activated : ezTriggerState::Deactivated;
      te.m_hTriggerComponent = pTriggerComponent->GetHandle();
      te.m_hOtherComponent = pOtherComponent->GetHandle();
    }
  }
}

void ezPhysXWorldModule::HandleSimulationEvents()
{
  SpawnPhysicsImpactReactions();
  UpdatePhysicsSlideAndRollReactions();
}

void ezPhysXWorldModule::SpawnPhysicsImpactReactions()
{
  // TODO: sort by impulse, cluster by position, only execute the first N contacts, prevent duplicate spawns at same location within short time

  for (const auto& ic : m_pSimulationEventCallback->m_InteractionContacts)
  {
    ic.m_pSurface->InteractWithSurface(m_pWorld, ezGameObjectHandle(), ic.m_vPosition, ic.m_vNormal, -ic.m_vNormal, ic.m_sInteraction, nullptr, ic.m_fImpulseSqr);
  }

  m_pSimulationEventCallback->m_InteractionContacts.Clear();
}

void ezPhysXWorldModule::UpdatePhysicsSlideAndRollReactions()
{
  for (auto& itSlide : m_pSimulationEventCallback->m_SlidingActors)
  {
    // ic.m_pSurface->InteractWithSurface(m_pWorld, ezGameObjectHandle(), ic.m_vPosition, ic.m_vNormal, -ic.m_vNormal, ic.m_sInteraction, nullptr,
    // ic.m_fImpulseSqr);

    auto& slideInfo = itSlide.Value();

    if (slideInfo.m_bStillRolling)
    {
      if (slideInfo.m_bStartedRolling == false)
      {
        slideInfo.m_bStartedRolling = true;
        // ezLog::Dev("Started Rolling");

        // TODO: make roll reaction configurable
        ezPrefabResourceHandle hPrefab = ezResourceManager::LoadResource<ezPrefabResource>("{ 4d306cc5-c1e6-4ec9-a04d-b804e3755210 }");
        ezResourceLock<ezPrefabResource> pPrefab(hPrefab, ezResourceAcquireMode::AllowLoadingFallback_NeverFail);
        if (pPrefab.GetAcquireResult() == ezResourceAcquireResult::Final)
        {
          ezHybridArray<ezGameObject*, 8> created;

          ezPrefabInstantiationOptions options;
          options.m_pCreatedRootObjectsOut = &created;
          options.bForceDynamic = true;

          pPrefab->InstantiatePrefab(*m_pWorld, ezTransform(slideInfo.m_vPosition), options);
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
        // ezLog::Dev("Stopped Rolling");
      }
    }

    if (slideInfo.m_bStillSliding)
    {
      if (slideInfo.m_bStartedSliding == false)
      {
        slideInfo.m_bStartedSliding = true;
        // ezLog::Dev("Started Sliding");

        // TODO: make slide reaction configurable
        ezPrefabResourceHandle hPrefab = ezResourceManager::LoadResource<ezPrefabResource>("{ c2d8d66d-b123-4cf1-b123-4d015fc69fb0 }");
        ezResourceLock<ezPrefabResource> pPrefab(hPrefab, ezResourceAcquireMode::AllowLoadingFallback_NeverFail);
        if (pPrefab.GetAcquireResult() == ezResourceAcquireResult::Final)
        {
          ezHybridArray<ezGameObject*, 8> created;

          ezPrefabInstantiationOptions options;
          options.m_pCreatedRootObjectsOut = &created;
          options.bForceDynamic = true;

          pPrefab->InstantiatePrefab(*m_pWorld, ezTransform(slideInfo.m_vPosition), options);
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
        // ezLog::Dev("Stopped Sliding");
      }
    }
  }
}
