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

  EZ_PROFILE_SCOPE("onContact");

  bool bSendContactReport = false;

  for (ezUInt32 uiPairIndex = 0; uiPairIndex < nbPairs; ++uiPairIndex)
  {
    const PxContactPair& pair = pairs[uiPairIndex];

    PxContactPairPoint contactPointBuffer[8];
    const ezUInt32 uiNumContactPoints = pair.extractContacts(contactPointBuffer, 8);

    ezBitflags<ezOnPhysXContact> ContactFlags0;
    ContactFlags0.SetValue(pair.shapes[0]->getSimulationFilterData().word3);
    ezBitflags<ezOnPhysXContact> ContactFlags1;
    ContactFlags1.SetValue(pair.shapes[1]->getSimulationFilterData().word3);

    ezBitflags<ezOnPhysXContact> CombinedContactFlags;
    CombinedContactFlags.SetValue(pair.shapes[0]->getSimulationFilterData().word3 | pair.shapes[1]->getSimulationFilterData().word3);

    bSendContactReport = bSendContactReport || CombinedContactFlags.IsSet(ezOnPhysXContact::SendReportMsg);

    if (CombinedContactFlags.IsAnySet(ezOnPhysXContact::AllReactions))
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

      if (!pair.flags.isSet(PxContactPairFlag::eACTOR_PAIR_HAS_FIRST_TOUCH))
      {
        OnContact_SlideAndRollReaction(pairHeader, ContactFlags0, vAvgPos, vAvgNormal, ContactFlags1, uiNumContactPoints, CombinedContactFlags);
      }

      if (fMaxImpactSqr >= 4.0f && CombinedContactFlags.IsAnySet(ezOnPhysXContact::ImpactReactions))
      {
        OnContact_ImpactReaction(contactPointBuffer, pair, vAvgPos, vAvgNormal, fMaxImpactSqr, pairHeader);
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
  EZ_PROFILE_SCOPE("HandleSimulationEvents");

  SpawnPhysicsImpactReactions();
  UpdatePhysicsSlideReactions();
  UpdatePhysicsRollReactions();
}

void ezPhysXWorldModule::SpawnPhysicsImpactReactions()
{
  EZ_PROFILE_SCOPE("SpawnPhysicsImpactReactions");

  // TODO (maybe): cluster by position, prevent duplicate spawns at same location within short time

  ezUInt32 uiMaxPrefabsToSpawn = 4;

  for (const auto& ic : m_pSimulationEventCallback->m_InteractionContacts)
  {
    if (ic.m_pSurface != nullptr)
    {
      if (ic.m_pSurface->InteractWithSurface(m_pWorld, ezGameObjectHandle(), ic.m_vPosition, ic.m_vNormal, -ic.m_vNormal, ic.m_sInteraction, nullptr, ic.m_fImpulseSqr))
      {
        if (--uiMaxPrefabsToSpawn == 0)
        {
          break;
        }
      }
    }
  }

  m_pSimulationEventCallback->m_InteractionContacts.Clear();
}

void ezPhysXWorldModule::UpdatePhysicsSlideReactions()
{
  EZ_PROFILE_SCOPE("UpdatePhysicsSlideReactions");

  for (auto& itSlide : m_pSimulationEventCallback->m_SlidingOrRollingActors)
  {
    auto& slideInfo = itSlide.Value();

    if (slideInfo.m_bStillSliding)
    {
      if (slideInfo.m_hSlidePrefab.IsInvalidated())
      {
        // TODO: make slide reaction configurable
        ezPrefabResourceHandle hPrefab = ezResourceManager::LoadResource<ezPrefabResource>("{ c2d8d66d-b123-4cf1-b123-4d015fc69fb0 }");
        ezResourceLock<ezPrefabResource> pPrefab(hPrefab, ezResourceAcquireMode::AllowLoadingFallback_NeverFail);
        if (pPrefab.GetAcquireResult() == ezResourceAcquireResult::Final)
        {
          ezHybridArray<ezGameObject*, 8> created;

          ezPrefabInstantiationOptions options;
          options.m_pCreatedRootObjectsOut = &created;
          options.bForceDynamic = true;

          pPrefab->InstantiatePrefab(*m_pWorld, ezTransform(slideInfo.m_vContactPosition), options);
          slideInfo.m_hSlidePrefab = created[0]->GetHandle();
        }
      }
      else
      {
        ezGameObject* pObject;
        if (m_pWorld->TryGetObject(slideInfo.m_hSlidePrefab, pObject))
        {
          pObject->SetGlobalPosition(slideInfo.m_vContactPosition);
        }
        else
        {
          slideInfo.m_hSlidePrefab.Invalidate();
        }
      }

      slideInfo.m_bStillSliding = false;
    }
    else
    {
      if (!slideInfo.m_hSlidePrefab.IsInvalidated())
      {
        m_pWorld->DeleteObjectDelayed(slideInfo.m_hSlidePrefab);
        slideInfo.m_hSlidePrefab.Invalidate();
      }

      // TODO: remove element from m_SlidingActors
    }
  }
}

void ezPhysXWorldModule::UpdatePhysicsRollReactions()
{
  EZ_PROFILE_SCOPE("UpdatePhysicsRollReactions");

  for (auto& itRoll : m_pSimulationEventCallback->m_SlidingOrRollingActors)
  {
    auto& rollInfo = itRoll.Value();

    if (rollInfo.m_bStillRolling)
    {
      if (rollInfo.m_hRollPrefab.IsInvalidated())
      {
        // TODO: make roll reaction configurable
        ezPrefabResourceHandle hPrefab = ezResourceManager::LoadResource<ezPrefabResource>("{ 4d306cc5-c1e6-4ec9-a04d-b804e3755210 }");
        ezResourceLock<ezPrefabResource> pPrefab(hPrefab, ezResourceAcquireMode::AllowLoadingFallback_NeverFail);
        if (pPrefab.GetAcquireResult() == ezResourceAcquireResult::Final)
        {
          ezHybridArray<ezGameObject*, 8> created;

          ezPrefabInstantiationOptions options;
          options.m_pCreatedRootObjectsOut = &created;
          options.bForceDynamic = true;

          pPrefab->InstantiatePrefab(*m_pWorld, ezTransform(rollInfo.m_vContactPosition), options);
          rollInfo.m_hRollPrefab = created[0]->GetHandle();
        }
      }
      else
      {
        ezGameObject* pObject;
        if (m_pWorld->TryGetObject(rollInfo.m_hRollPrefab, pObject))
        {
          pObject->SetGlobalPosition(rollInfo.m_vContactPosition);
        }
        else
        {
          rollInfo.m_hRollPrefab.Invalidate();
        }
      }

      rollInfo.m_bStillRolling = false;
    }
    else
    {
      if (!rollInfo.m_hRollPrefab.IsInvalidated())
      {
        m_pWorld->DeleteObjectDelayed(rollInfo.m_hRollPrefab);
        rollInfo.m_hRollPrefab.Invalidate();
      }

      // TODO: remove element from m_RollingActors
    }
  }
}

void ezPxSimulationEventCallback::OnContact_ImpactReaction(PxContactPairPoint* contactPointBuffer, const PxContactPair& pair, const ezVec3& vAvgPos, const ezVec3& vAvgNormal, float fMaxImpactSqr, const PxContactPairHeader& pairHeader)
{
  const PxContactPairPoint& point = contactPointBuffer[0];

  const float fDistanceSqr = (vAvgPos - m_vMainCameraPosition).GetLengthSquared();

  InteractionContact* ic = nullptr;

  if (m_InteractionContacts.GetCount() < m_InteractionContacts.GetCapacity())
  {
    ic = &m_InteractionContacts.ExpandAndGetRef();
    ic->m_pSurface = nullptr;
    ic->m_fDistanceSqr = ezMath::HighValue<float>();
  }
  else
  {
    // compute a score, which contact point is best to replace
    // * prefer to replace points that are farther away than the new one
    // * prefer to replace points that have a lower impact strength than the new one

    float fBestScore = 0;
    ezUInt32 uiBestScore = 0;

    for (ezUInt32 i = 0; i < m_InteractionContacts.GetCount(); ++i)
    {
      float fScore = 0;
      fScore += m_InteractionContacts[i].m_fDistanceSqr - fDistanceSqr;
      fScore += ezMath::Square(fMaxImpactSqr - m_InteractionContacts[i].m_fImpulseSqr);

      if (fScore > fBestScore)
      {
        fBestScore = fScore;
        uiBestScore = i;
      }
    }

    // this is the best candidate to replace
    ic = &m_InteractionContacts[uiBestScore];
  }


  if (PxMaterial* pMaterial0 = pair.shapes[0]->getMaterialFromInternalFaceIndex(point.internalFaceIndex0))
  {
    if (PxMaterial* pMaterial1 = pair.shapes[1]->getMaterialFromInternalFaceIndex(point.internalFaceIndex1))
    {
      if (ezSurfaceResource* pSurface0 = ezPxUserData::GetSurfaceResource(pMaterial0->userData))
      {
        if (ezSurfaceResource* pSurface1 = ezPxUserData::GetSurfaceResource(pMaterial1->userData))
        {
          ic->m_fDistanceSqr = fDistanceSqr;
          ic->m_vPosition = vAvgPos;
          ic->m_vNormal = vAvgNormal;
          ic->m_vNormal.NormalizeIfNotZero(ezVec3(0, 0, 1)).IgnoreResult();
          ic->m_fImpulseSqr = fMaxImpactSqr;

          // if one actor is static or kinematic, prefer to spawn the interaction from its surface definition
          if (pairHeader.actors[0]->getType() == PxActorType::eRIGID_STATIC || (pairHeader.actors[0]->getType() == PxActorType::eRIGID_DYNAMIC && static_cast<PxRigidDynamic*>(pairHeader.actors[0])->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC)))
          {
            ic->m_pSurface = pSurface0;
            ic->m_sInteraction = pSurface1->GetDescriptor().m_sOnCollideInteraction;
          }
          else
          {
            ic->m_pSurface = pSurface1;
            ic->m_sInteraction = pSurface0->GetDescriptor().m_sOnCollideInteraction;
          }

          return;
        }
      }
    }
  }
}

void ezPxSimulationEventCallback::OnContact_RollReaction(const ezVec3& vAvgPos, PxRigidDynamic* pRigid0, PxRigidDynamic* pRigid1, ezBitflags<ezOnPhysXContact>& ContactFlags0, ezBitflags<ezOnPhysXContact>& ContactFlags1)
{
  if (/*false &&*/ pRigid0 && ContactFlags0.IsAnySet(ezOnPhysXContact::AllRollReactions))
  {
    const ezVec3 vAngularVel = ezPxConversionUtils::ToVec3(pRigid0->getGlobalPose().rotateInv(pRigid0->getAngularVelocity()));

    // TODO: make threshold tweakable
    if ((ContactFlags0.IsSet(ezOnPhysXContact::RollXReactions) && ezMath::Abs(vAngularVel.x) > 1.0f) || (ContactFlags0.IsSet(ezOnPhysXContact::RollYReactions) && ezMath::Abs(vAngularVel.y) > 1.0f) ||
        (ContactFlags0.IsSet(ezOnPhysXContact::RollZReactions) && ezMath::Abs(vAngularVel.z) > 1.0f))
    {
      m_SlidingOrRollingActors[pRigid0].m_bStillRolling = true;
      m_SlidingOrRollingActors[pRigid0].m_vContactPosition = vAvgPos;
    }
  }

  if (/*false &&*/ pRigid1 && ContactFlags1.IsAnySet(ezOnPhysXContact::AllRollReactions))
  {
    const ezVec3 vAngularVel = ezPxConversionUtils::ToVec3(pRigid1->getGlobalPose().rotateInv(pRigid1->getAngularVelocity()));

    // TODO: make threshold tweakable
    if ((ContactFlags1.IsSet(ezOnPhysXContact::RollXReactions) && ezMath::Abs(vAngularVel.x) > 1.0f) || (ContactFlags1.IsSet(ezOnPhysXContact::RollYReactions) && ezMath::Abs(vAngularVel.y) > 1.0f) ||
        (ContactFlags1.IsSet(ezOnPhysXContact::RollZReactions) && ezMath::Abs(vAngularVel.z) > 1.0f))
    {
      m_SlidingOrRollingActors[pRigid1].m_bStillRolling = true;
      m_SlidingOrRollingActors[pRigid1].m_vContactPosition = vAvgPos;
    }
  }
}

void ezPxSimulationEventCallback::OnContact_SlideReaction(const ezVec3& vAvgPos, const ezVec3& vAvgNormal0, PxRigidDynamic* pRigid0, PxRigidDynamic* pRigid1, ezBitflags<ezOnPhysXContact>& ContactFlags0, ezBitflags<ezOnPhysXContact>& ContactFlags1, ezBitflags<ezOnPhysXContact>& CombinedContactFlags)
{
  ezVec3 vVelocity0(0);
  ezVec3 vVelocity1(0);

  if (pRigid0)
  {
    vVelocity0 = ezPxConversionUtils::ToVec3(pRigid0->getLinearVelocity());

    if (!vVelocity0.IsValid())
      vVelocity0.SetZero();
  }

  if (pRigid1)
  {
    vVelocity1 = ezPxConversionUtils::ToVec3(pRigid1->getLinearVelocity());

    if (!vVelocity1.IsValid())
      vVelocity1.SetZero();
  }

  const ezVec3 vRelativeVelocity = vVelocity1 - vVelocity0;

  if (!vRelativeVelocity.IsZero(0.0001f))
  {
    const ezVec3 vRelativeVelocityDir = vRelativeVelocity.GetNormalized();

    ezVec3 vAvgNormal = vAvgNormal0;
    vAvgNormal.NormalizeIfNotZero(ezVec3::UnitZAxis()).IgnoreResult();

    if (ezMath::Abs(vAvgNormal.Dot(vRelativeVelocityDir)) < 0.1f)
    {
      // TODO: make threshold tweakable
      if (vRelativeVelocity.GetLengthSquared() > ezMath::Square(1.0f))
      {
        if (pRigid0 && ContactFlags0.IsAnySet(ezOnPhysXContact::SlideReactions))
        {
          auto& info = m_SlidingOrRollingActors[pRigid0];

          if (!info.m_bStillRolling)
          {
            info.m_bStillSliding = true;
            info.m_vContactPosition = vAvgPos;
          }
        }

        if (pRigid1 && ContactFlags1.IsAnySet(ezOnPhysXContact::SlideReactions))
        {
          auto& info = m_SlidingOrRollingActors[pRigid1];

          if (!info.m_bStillRolling)
          {
            info.m_bStillSliding = true;
            info.m_vContactPosition = vAvgPos;
          }
        }
      }
    }
  }
}

void ezPxSimulationEventCallback::OnContact_SlideAndRollReaction(const PxContactPairHeader& pairHeader, ezBitflags<ezOnPhysXContact>& ContactFlags0, const ezVec3& vAvgPos, const ezVec3& vAvgNormal, ezBitflags<ezOnPhysXContact>& ContactFlags1, const ezUInt32 uiNumContactPoints, ezBitflags<ezOnPhysXContact>& CombinedContactFlags)
{
  PxRigidDynamic* pRigid0 = nullptr;
  PxRigidDynamic* pRigid1 = nullptr;

  if (pairHeader.actors[0]->getType() == PxActorType::eRIGID_DYNAMIC)
  {
    pRigid0 = static_cast<PxRigidDynamic*>(pairHeader.actors[0]);
  }

  if (pairHeader.actors[1]->getType() == PxActorType::eRIGID_DYNAMIC)
  {
    pRigid1 = static_cast<PxRigidDynamic*>(pairHeader.actors[1]);
  }

  if (/*false &&*/ uiNumContactPoints >= 2 && CombinedContactFlags.IsAnySet(ezOnPhysXContact::SlideReactions))
  {
    OnContact_SlideReaction(vAvgPos, vAvgNormal, pRigid0, pRigid1, ContactFlags0, ContactFlags1, CombinedContactFlags);
  }

  if (CombinedContactFlags.IsAnySet(ezOnPhysXContact::AllRollReactions))
  {
    OnContact_RollReaction(vAvgPos, pRigid0, pRigid1, ContactFlags0, ContactFlags1);
  }
}
