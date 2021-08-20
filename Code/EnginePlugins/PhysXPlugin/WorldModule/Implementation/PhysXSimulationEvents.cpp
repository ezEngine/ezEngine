#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/Messages/CollisionMessage.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Configuration/CVar.h>
#include <PhysXPlugin/Components/PxTriggerComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>

ezCVarInt cvar_PhysicsReactionsMaxImpacts("Physics.Reactions.MaxImpacts", 4, ezCVarFlags::Default, "Maximum number of impact reactions to spawn per frame.");
ezCVarInt cvar_PhysicsReactionsMaxSlidesOrRolls("Physics.Reactions.MaxSlidesOrRolls", 4, ezCVarFlags::Default, "Maximum number of active slide or roll reactions.");
ezCVarBool cvar_PhysicsReactionsVisImpacts("Physics.Reactions.VisImpacts", false, ezCVarFlags::Default, "Visualize where impact reactions are spawned.");
ezCVarBool cvar_PhysicsReactionsVisDiscardedImpacts("Physics.Reactions.VisDiscardedImpacts", false, ezCVarFlags::Default, "Visualize where impact reactions were NOT spawned.");
ezCVarBool cvar_PhysicsReactionsVisSlides("Physics.Reactions.VisSlides", false, ezCVarFlags::Default, "Visualize active slide reactions.");
ezCVarBool cvar_PhysicsReactionsVisRolls("Physics.Reactions.VisRolls", false, ezCVarFlags::Default, "Visualize active roll reactions.");

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

    if (pair.flags & (PxContactPairFlag::eREMOVED_SHAPE_0 | PxContactPairFlag::eREMOVED_SHAPE_1))
      continue;

    PxContactPairPoint contactPointBuffer[8];
    const ezUInt32 uiNumContactPoints = pair.extractContacts(contactPointBuffer, 8);

    ezBitflags<ezOnPhysXContact> ContactFlags0;
    ContactFlags0.SetValue(pair.shapes[0]->getSimulationFilterData().word3);
    ezBitflags<ezOnPhysXContact> ContactFlags1;
    ContactFlags1.SetValue(pair.shapes[1]->getSimulationFilterData().word3);

    ezBitflags<ezOnPhysXContact> CombinedContactFlags;
    CombinedContactFlags.SetValue(ContactFlags0.GetValue() | ContactFlags1.GetValue());

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
        OnContact_SlideAndRollReaction(pairHeader, pair, ContactFlags0, vAvgPos, vAvgNormal, ContactFlags1, uiNumContactPoints, CombinedContactFlags);
      }

      if (fMaxImpactSqr >= 1.0f && CombinedContactFlags.IsAnySet(ezOnPhysXContact::ImpactReactions))
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
  UpdatePhysicsRollReactions();
  UpdatePhysicsSlideReactions();

  for (auto& info : m_pSimulationEventCallback->m_SlidingOrRollingActors)
  {
    if (info.m_hRollPrefab.IsInvalidated() && info.m_hSlidePrefab.IsInvalidated())
    {
      // mark this slot as ready for reuse
      info.m_pActor = nullptr;
    }
  }

  // slowly shrink the array
  if (!m_pSimulationEventCallback->m_SlidingOrRollingActors.IsEmpty() && m_pSimulationEventCallback->m_SlidingOrRollingActors.PeekBack().m_pActor == nullptr)
  {
    m_pSimulationEventCallback->m_SlidingOrRollingActors.PopBack();
  }
}

void ezPhysXWorldModule::SpawnPhysicsImpactReactions()
{
  EZ_PROFILE_SCOPE("SpawnPhysicsImpactReactions");

  ezUInt32 uiMaxPrefabsToSpawn = cvar_PhysicsReactionsMaxImpacts;

  for (const auto& ic : m_pSimulationEventCallback->m_InteractionContacts)
  {
    if (ic.m_pSurface != nullptr)
    {
      if (uiMaxPrefabsToSpawn > 0 && ic.m_pSurface->InteractWithSurface(m_pWorld, ezGameObjectHandle(), ic.m_vPosition, ic.m_vNormal, -ic.m_vNormal, ic.m_sInteraction, nullptr, ic.m_fImpulseSqr))
      {
        --uiMaxPrefabsToSpawn;

        if (cvar_PhysicsReactionsVisImpacts)
        {
          ezDebugRenderer::AddPersistentCross(GetWorld(), 1.0f, ezColor::LightGreen, ezTransform(ic.m_vPosition), ezTime::Seconds(3));
        }
      }
      else
      {
        if (cvar_PhysicsReactionsVisDiscardedImpacts)
        {
          ezDebugRenderer::AddPersistentCross(GetWorld(), 1.0f, ezColor::DarkGray, ezTransform(ic.m_vPosition), ezTime::Seconds(1));
        }
      }
    }
  }

  m_pSimulationEventCallback->m_InteractionContacts.Clear();
}

void ezPhysXWorldModule::UpdatePhysicsSlideReactions()
{
  EZ_PROFILE_SCOPE("UpdatePhysicsSlideReactions");

  for (auto& slideInfo : m_pSimulationEventCallback->m_SlidingOrRollingActors)
  {
    if (slideInfo.m_pActor == nullptr)
      continue;

    if (slideInfo.m_bStillSliding)
    {
      if (slideInfo.m_hSlidePrefab.IsInvalidated())
      {
        ezPrefabResourceHandle hPrefab = ezResourceManager::LoadResource<ezPrefabResource>(slideInfo.m_sSlideInteractionPrefab);
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

      if (cvar_PhysicsReactionsVisSlides)
      {
        ezDebugRenderer::DrawLineBox(GetWorld(), ezBoundingBox(ezVec3(-0.5f), ezVec3(0.5f)), ezColor::BlueViolet, ezTransform(slideInfo.m_vContactPosition));
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
    }
  }
}

void ezPhysXWorldModule::UpdatePhysicsRollReactions()
{
  EZ_PROFILE_SCOPE("UpdatePhysicsRollReactions");

  for (auto& rollInfo : m_pSimulationEventCallback->m_SlidingOrRollingActors)
  {
    if (rollInfo.m_pActor == nullptr)
      continue;

    if (rollInfo.m_bStillRolling)
    {
      if (rollInfo.m_hRollPrefab.IsInvalidated())
      {
        ezPrefabResourceHandle hPrefab = ezResourceManager::LoadResource<ezPrefabResource>(rollInfo.m_sRollInteractionPrefab);
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

      if (cvar_PhysicsReactionsVisRolls)
      {
        ezDebugRenderer::DrawLineCapsuleZ(GetWorld(), 0.4f, 0.2f, ezColor::GreenYellow, ezTransform(rollInfo.m_vContactPosition));
      }

      rollInfo.m_bStillRolling = false;
      rollInfo.m_bStillSliding = false; // ensures that no slide reaction is spawned as well
    }
    else
    {
      if (!rollInfo.m_hRollPrefab.IsInvalidated())
      {
        m_pWorld->DeleteObjectDelayed(rollInfo.m_hRollPrefab);
        rollInfo.m_hRollPrefab.Invalidate();
      }
    }
  }
}

void ezPxSimulationEventCallback::OnContact_ImpactReaction(PxContactPairPoint* contactPointBuffer, const PxContactPair& pair, const ezVec3& vAvgPos, const ezVec3& vAvgNormal, float fMaxImpactSqr, const PxContactPairHeader& pairHeader)
{
  const PxContactPairPoint& point = contactPointBuffer[0];

  const float fDistanceSqr = (vAvgPos - m_vMainCameraPosition).GetLengthSquared();

  InteractionContact* ic = nullptr;

  if (m_InteractionContacts.GetCount() < cvar_PhysicsReactionsMaxImpacts * 2)
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
    ezUInt32 uiBestScore = 0xFFFFFFFFu;

    for (ezUInt32 i = 0; i < m_InteractionContacts.GetCount(); ++i)
    {
      float fScore = 0;
      fScore += m_InteractionContacts[i].m_fDistanceSqr - fDistanceSqr;
      fScore += 2.0f * (fMaxImpactSqr - m_InteractionContacts[i].m_fImpulseSqr);

      if (fScore > fBestScore)
      {
        fBestScore = fScore;
        uiBestScore = i;
      }
    }

    if (uiBestScore == 0xFFFFFFFFu)
    {
      if (cvar_PhysicsReactionsVisDiscardedImpacts)
      {
        ezDebugRenderer::AddPersistentCross(m_pWorld, 1.0f, ezColor::DimGrey, ezTransform(vAvgPos), ezTime::Seconds(3));
      }

      return;
    }
    else
    {
      if (cvar_PhysicsReactionsVisDiscardedImpacts)
      {
        ezDebugRenderer::AddPersistentCross(m_pWorld, 1.0f, ezColor::DimGrey, ezTransform(m_InteractionContacts[uiBestScore].m_vPosition), ezTime::Seconds(3));
      }
    }

    // this is the best candidate to replace
    ic = &m_InteractionContacts[uiBestScore];
  }

  PxMaterial* pMaterial0 = pair.shapes[0]->getMaterialFromInternalFaceIndex(point.internalFaceIndex0);
  PxMaterial* pMaterial1 = pair.shapes[1]->getMaterialFromInternalFaceIndex(point.internalFaceIndex1);

  if (pMaterial0 || pMaterial1)
  {
    // if one of the objects doesn't have a surface configured, use the other one
    if (pMaterial0 == nullptr)
      pMaterial0 = pMaterial1;
    if (pMaterial1 == nullptr)
      pMaterial1 = pMaterial0;

    ezSurfaceResource* pSurface0 = ezPxUserData::GetSurfaceResource(pMaterial0->userData);
    ezSurfaceResource* pSurface1 = ezPxUserData::GetSurfaceResource(pMaterial1->userData);

    if (pSurface0 || pSurface1)
    {
      // if one of the objects doesn't have a surface configured, use the other one
      if (pSurface0 == nullptr)
        pSurface0 = pSurface1;
      if (pSurface1 == nullptr)
        pSurface1 = pSurface0;

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

  if (cvar_PhysicsReactionsVisDiscardedImpacts)
  {
    ezDebugRenderer::AddPersistentCross(m_pWorld, 1.0f, ezColor::DarkOrange, ezTransform(vAvgPos), ezTime::Seconds(10));
  }
}

ezPxSimulationEventCallback::SlideAndRollInfo* ezPxSimulationEventCallback::FindSlideOrRollInfo(PxRigidDynamic* pActor, const ezVec3& vAvgPos)
{
  EZ_ASSERT_DEV(pActor != nullptr, "");

  SlideAndRollInfo* pUnused = nullptr;

  for (auto& info : m_SlidingOrRollingActors)
  {
    if (info.m_pActor == pActor)
      return &info;

    if (info.m_pActor == nullptr)
      pUnused = &info;
  }

  const float fDistSqr = (vAvgPos - m_vMainCameraPosition).GetLengthSquared();

  if (pUnused != nullptr)
  {
    pUnused->m_fDistanceSqr = fDistSqr;
    pUnused->m_pActor = pActor;
    return pUnused;
  }

  if (m_SlidingOrRollingActors.GetCount() < cvar_PhysicsReactionsMaxSlidesOrRolls)
  {
    pUnused = &m_SlidingOrRollingActors.ExpandAndGetRef();
    pUnused->m_fDistanceSqr = fDistSqr;
    pUnused->m_pActor = pActor;
    return pUnused;
  }

  float fBestDist = 0.0f;

  for (auto& info : m_SlidingOrRollingActors)
  {
    if (!info.m_hRollPrefab.IsInvalidated() || !info.m_hSlidePrefab.IsInvalidated())
      continue;

    // this slot is not yet really in use, so can be replaced by a better match

    if (fDistSqr < info.m_fDistanceSqr && info.m_fDistanceSqr > fBestDist)
    {
      fBestDist = info.m_fDistanceSqr;
      pUnused = &info;
    }
  }

  if (pUnused != nullptr)
  {
    pUnused->m_fDistanceSqr = fDistSqr;
    pUnused->m_pActor = pActor;
    return pUnused;
  }

  return nullptr;
}

void ezPxSimulationEventCallback::OnContact_RollReaction(const ezVec3& vAvgPos, PxRigidDynamic* pRigid0, PxRigidDynamic* pRigid1, const PxContactPair& pair, ezBitflags<ezOnPhysXContact>& ContactFlags0, ezBitflags<ezOnPhysXContact>& ContactFlags1)
{
  // only consider something 'rolling' when it turns faster than this (per second)
  constexpr ezAngle rollThreshold = ezAngle::Degree(45);

  PxRigidDynamic* pRigid[2] = {pRigid0, pRigid1};
  ezBitflags<ezOnPhysXContact> contactFlags[2] = {ContactFlags0, ContactFlags1};

  for (ezUInt32 i = 0; i < 2; ++i)
  {
    if (pRigid[i] && contactFlags[i].IsAnySet(ezOnPhysXContact::AllRollReactions))
    {
      const ezVec3 vAngularVel = ezPxConversionUtils::ToVec3(pRigid[i]->getGlobalPose().rotateInv(pRigid[i]->getAngularVelocity()));

      if ((contactFlags[i].IsSet(ezOnPhysXContact::RollXReactions) && ezMath::Abs(vAngularVel.x) > rollThreshold.GetRadian()) ||
          (contactFlags[i].IsSet(ezOnPhysXContact::RollYReactions) && ezMath::Abs(vAngularVel.y) > rollThreshold.GetRadian()) ||
          (contactFlags[i].IsSet(ezOnPhysXContact::RollZReactions) && ezMath::Abs(vAngularVel.z) > rollThreshold.GetRadian()))
      {
        if (PxMaterial* pMaterial = pair.shapes[i]->getMaterialFromInternalFaceIndex(0))
        {
          if (ezSurfaceResource* pSurface = ezPxUserData::GetSurfaceResource(pMaterial->userData))
          {
            if (!pSurface->GetDescriptor().m_sRollInteractionPrefab.IsEmpty())
            {
              if (auto pInfo = FindSlideOrRollInfo(pRigid[i], vAvgPos))
              {
                pInfo->m_bStillRolling = true;
                pInfo->m_vContactPosition = vAvgPos;
                pInfo->m_sRollInteractionPrefab = pSurface->GetDescriptor().m_sRollInteractionPrefab;
              }
            }
          }
        }
      }
    }
  }
}

void ezPxSimulationEventCallback::OnContact_SlideReaction(const ezVec3& vAvgPos, const ezVec3& vAvgNormal0, PxRigidDynamic* pRigid0, PxRigidDynamic* pRigid1, const PxContactPair& pair, ezBitflags<ezOnPhysXContact>& contactFlags0, ezBitflags<ezOnPhysXContact>& contactFlags1)
{
  ezVec3 vVelocity[2] = {ezVec3::ZeroVector(), ezVec3::ZeroVector()};

  if (pRigid0)
  {
    vVelocity[0] = ezPxConversionUtils::ToVec3(pRigid0->getLinearVelocity());

    if (!vVelocity[0].IsValid())
      vVelocity[0].SetZero();
  }

  if (pRigid1)
  {
    vVelocity[1] = ezPxConversionUtils::ToVec3(pRigid1->getLinearVelocity());

    if (!vVelocity[1].IsValid())
      vVelocity[1].SetZero();
  }

  const ezVec3 vRelativeVelocity = vVelocity[1] - vVelocity[0];

  if (!vRelativeVelocity.IsZero(0.0001f))
  {
    const ezVec3 vRelativeVelocityDir = vRelativeVelocity.GetNormalized();

    ezVec3 vAvgNormal = vAvgNormal0;
    vAvgNormal.NormalizeIfNotZero(ezVec3::UnitZAxis()).IgnoreResult();

    // an object is only 'sliding' if it moves at roughly 90 degree along another object
    constexpr float slideAngle = 0.17f; // ezMath ::Cos(ezAngle::Degree(80));

    if (ezMath::Abs(vAvgNormal.Dot(vRelativeVelocityDir)) < slideAngle)
    {
      constexpr float slideSpeedThreshold = 0.5f; // in meters per second

      if (vRelativeVelocity.GetLengthSquared() > ezMath::Square(slideSpeedThreshold))
      {
        PxRigidDynamic* pRigid[2] = {pRigid0, pRigid1};
        ezBitflags<ezOnPhysXContact> contactFlags[2] = {contactFlags0, contactFlags1};

        for (ezUInt32 i = 0; i < 2; ++i)
        {
          if (pRigid[i] && contactFlags[i].IsAnySet(ezOnPhysXContact::SlideReactions))
          {
            if (PxMaterial* pMaterial = pair.shapes[i]->getMaterialFromInternalFaceIndex(0))
            {
              if (ezSurfaceResource* pSurface = ezPxUserData::GetSurfaceResource(pMaterial->userData))
              {
                if (!pSurface->GetDescriptor().m_sSlideInteractionPrefab.IsEmpty())
                {
                  if (auto pInfo = FindSlideOrRollInfo(pRigid[i], vAvgPos))
                  {
                    if (!pInfo->m_bStillRolling)
                    {
                      pInfo->m_bStillSliding = true;
                      pInfo->m_vContactPosition = vAvgPos;
                      pInfo->m_sSlideInteractionPrefab = pSurface->GetDescriptor().m_sSlideInteractionPrefab;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

void ezPxSimulationEventCallback::OnContact_SlideAndRollReaction(const PxContactPairHeader& pairHeader, const PxContactPair& pair, ezBitflags<ezOnPhysXContact>& ContactFlags0, const ezVec3& vAvgPos, const ezVec3& vAvgNormal, ezBitflags<ezOnPhysXContact>& ContactFlags1, const ezUInt32 uiNumContactPoints, ezBitflags<ezOnPhysXContact>& CombinedContactFlags)
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

  if (uiNumContactPoints >= 2 && CombinedContactFlags.IsAnySet(ezOnPhysXContact::SlideReactions))
  {
    OnContact_SlideReaction(vAvgPos, vAvgNormal, pRigid0, pRigid1, pair, ContactFlags0, ContactFlags1);
  }

  if (CombinedContactFlags.IsAnySet(ezOnPhysXContact::AllRollReactions))
  {
    OnContact_RollReaction(vAvgPos, pRigid0, pRigid1, pair, ContactFlags0, ContactFlags1);
  }
}
