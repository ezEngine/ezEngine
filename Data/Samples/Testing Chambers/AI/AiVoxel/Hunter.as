#include "AiShipBase.as"

enum HunterState
{
    Wandering, // no prey in range, exploring randomly
    Pursuing,  // prey in sight, navigating towards it
    Tracking,  // lost sight of prey, heading to its last known position
    TooClose,  // prey is within catch distance, stop and just rotate towards it
}

class Hunter : AiShipBase
{
    ezString PreyMarker = "Prey";

    private HunterState m_State = HunterState::Wandering;
    private ezGameObjectHandle m_hTrackedPrey;
    private ezVec3 m_vLastKnownPreyPos;
    private bool m_bHasLastKnownPos = false;

    private float m_fDetectionRange = 40.0f;
    private float m_fCatchDistance = 5.0f;

    void OnSimulationStarted()
    {
        GetOwner().MakeDynamic();
        m_vHomePos = GetOwner().GetGlobalPosition();
        m_fWanderRadius = 150.0f;
        m_fLife = 10.0f;
        SetUpdateInterval(ezTime::Milliseconds(60));
    }

    void Update(ezTime deltaTime)
    {
        ezAiVoxelNavigationComponent@ navComp;
        if (!GetOwner().TryGetComponentOfBaseType(@navComp))
            return;

        ezVec3 vOwnPos    = GetOwner().GetGlobalPosition();
        ezVec3 vStatusPos = vOwnPos + ezVec3(0, 0, 2.2f);

        if (TryRecoverFromFailedState(navComp, vOwnPos))
            return;

        // Re-check the prey we're already committed to (if any) - as long as it stays in
        // range, keep pursuing it exclusively, even if another prey gets closer meanwhile.
        ezGameObject@ trackedObj;
        if (!m_hTrackedPrey.IsInvalidated())
        {
            GetWorld().TryGetObject(m_hTrackedPrey, @trackedObj);
        }

        if (@trackedObj != null)
        {
            float fDist = vOwnPos.GetDistanceTo(trackedObj.GetGlobalPosition());

            if (fDist > m_fDetectionRange)
            {
                // Too far away to keep direct pursuit - remember where it was and switch to tracking.
                m_vLastKnownPreyPos = trackedObj.GetGlobalPosition();
                m_bHasLastKnownPos  = true;
                m_hTrackedPrey.Invalidate();
                navComp.CancelNavigation();
                @trackedObj = null;
            }
        }
        else if (!m_hTrackedPrey.IsInvalidated())
        {
            // Object got deleted while we were tracking it - it's gone for good, no point
            // in heading to its last known position.
            m_hTrackedPrey.Invalidate();
            m_bHasLastKnownPos = false;
            m_State = HunterState::Wandering;
            navComp.CancelNavigation();
        }

        if (@trackedObj != null)
        {
            m_vLastKnownPreyPos = trackedObj.GetGlobalPosition();
            m_bHasLastKnownPos  = true;

            float fDist = vOwnPos.GetDistanceTo(m_vLastKnownPreyPos);

            if (fDist <= m_fCatchDistance)
            {
                // Close enough - stop moving and just rotate towards the prey.
                m_State = HunterState::TooClose;
                navComp.CancelNavigation();
            }
            else
            {
                m_State = HunterState::Pursuing;
                // Let the component continuously track and re-path towards the prey object
                navComp.SetNavigationTarget(trackedObj.GetHandle());
            }

            if (ShowDebugInfo)
            {
                ezDebug::DrawLine(vOwnPos, m_vLastKnownPreyPos, ezColor::OrangeRed, ezColor::Orange);
            }
        }
        else if (m_bHasLastKnownPos)
        {
            bool bJustLostSight = (m_State != HunterState::Tracking);
            m_State = HunterState::Tracking;

            float fDist = vOwnPos.GetDistanceTo(m_vLastKnownPreyPos);
            if (fDist <= m_fCatchDistance + 0.5f)
            {
                // Reached the last known spot, prey is truly gone - give up and wander off
                m_bHasLastKnownPos = false;
                m_State = HunterState::Wandering;
                PickWanderDestination(navComp);
            }
            else if (bJustLostSight)
            {
                // Head to its last known position, just once - the component keeps moving
                // there on its own, no continuous target-tracking involved.
                navComp.SetDestination(m_vLastKnownPreyPos);
            }
            else if (!navComp.IsNavigating())
            {
                // Somehow ended up idle without reaching the last known position (e.g. the path
                // finished short of the catch-distance threshold, or pathfinding failed and
                // recovery placed us somewhere that doesn't retry on its own) - don't sit there
                // forever, give up tracking and fall back to wandering instead.
                m_bHasLastKnownPos = false;
                m_State = HunterState::Wandering;
                PickWanderDestination(navComp);
            }

            if (ShowDebugInfo)
            {
                ezDebug::DrawLine(vOwnPos, m_vLastKnownPreyPos, ezColor::Yellow, ezColor::OrangeRed);
            }

            // No longer directly pursuing anyone - look for a new, reachable prey to commit to.
            TryAcquireNewPrey(navComp, vOwnPos);
        }
        else
        {
            m_State = HunterState::Wandering;

            if (!TryAcquireNewPrey(navComp, vOwnPos) && (!navComp.IsNavigating() || navComp.IsApproachingDestination()))
            {
                PickWanderDestination(navComp);
            }
        }

        if (m_State == HunterState::TooClose && m_bHasLastKnownPos)
        {
            RotateTowards(vOwnPos, m_vLastKnownPreyPos);
        }

        if (ShowDebugInfo)
        {
            DrawDebugState(vStatusPos, vOwnPos);
        }

        UpdateGun(vOwnPos);
    }

    /// Enables the "Gun" child object while we have a target and it is within 30 degrees
    /// of our forward axis, disables it otherwise.
    void UpdateGun(ezVec3 vOwnPos)
    {
        ezGameObject@ gunObj = GetOwner().FindChildByName("Gun");
        if (@gunObj == null)
            return;

        bool bAimedAtTarget = false;

        if (m_bHasLastKnownPos)
        {
            ezVec3 dirToTarget = m_vLastKnownPreyPos - vOwnPos;
            dirToTarget.Normalize();

            bAimedAtTarget = dirToTarget.Dot(GetOwner().GetGlobalDirForwards()) > ezMath::Cos(ezAngle::MakeFromDegree(30));
        }

        gunObj.SetActiveFlag(bAimedAtTarget);
    }

    /// Center of the prey-detection sphere. Pushed half the detection range forward along the
    /// hunter's facing, so it mostly sees prey ahead of it and barely notices prey directly behind.
    ezVec3 GetDetectionCenter(ezVec3 vOwnPos)
    {
        return vOwnPos + GetOwner().GetGlobalDirForwards() * (m_fDetectionRange * 0.5f);
    }

    /// Looks for the closest prey in range and, if found, commits to pursuing it. Returns true if a
    /// prey was acquired.
    bool TryAcquireNewPrey(ezAiVoxelNavigationComponent@ navComp, ezVec3 vOwnPos)
    {
        ezGameObject@ preyObj = ezSpatial::FindClosestObjectInSphere(PreyMarker, GetDetectionCenter(vOwnPos), m_fDetectionRange);
        if (@preyObj == null)
            return false;

        m_hTrackedPrey      = preyObj.GetHandle();
        m_vLastKnownPreyPos = preyObj.GetGlobalPosition();
        m_bHasLastKnownPos  = true;
        m_State             = HunterState::Pursuing;
        navComp.SetNavigationTarget(preyObj.GetHandle());
        return true;
    }

    /// Rotates the owner towards vTargetPos, without moving.
    void RotateTowards(ezVec3 vOwnPos, ezVec3 vTargetPos)
    {
        ezVec3 dirToTarget = vTargetPos - vOwnPos;
        if (dirToTarget.GetLength() < 0.01f)
            return;

        dirToTarget.Normalize();

        ezQuat targetRotation = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), dirToTarget);
        ezQuat newRotation    = ezQuat::MakeSlerp(GetOwner().GetGlobalRotation(), targetRotation, 0.1f);

        GetOwner().SetGlobalRotation(newRotation);
    }

    void DrawDebugState(ezVec3 vTextPos, ezVec3 vOwnPos)
    {
        if (m_State == HunterState::Wandering)
        {
            ezDebug::Draw3DText("Hunter: Wandering", vTextPos, ezColor::LightGrey, 24);
        }
        else if (m_State == HunterState::Pursuing)
        {
            ezDebug::Draw3DText("Hunter: Pursuing!", vTextPos, ezColor::OrangeRed, 28);
        }
        else if (m_State == HunterState::Tracking)
        {
            ezDebug::Draw3DText("Hunter: Tracking...", vTextPos, ezColor::Yellow, 28);
        }
        else
        {
            ezDebug::Draw3DText("Hunter: Too Close!", vTextPos, ezColor::Red, 36);
        }

        ezDebug::DrawLineSphere(GetDetectionCenter(vOwnPos), m_fDetectionRange, ezColor(1, 0.3f, 0, 0.3f));
    }
}
