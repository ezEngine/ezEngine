// Shared behavior for the voxel-navigation AI sample ships (Hunter and Prey).
//
// Holds the boilerplate common to both: taking damage / dying, recovering when pathfinding fails,
// and picking a random wander destination. The predator/prey behavior itself lives in the derived
// classes. Derived classes set m_vHomePos / m_fWanderRadius / m_fLife in their OnSimulationStarted().
abstract class AiShipBase : ezAngelScriptClass
{
    bool ShowDebugInfo = false;

    protected ezVec3 m_vHomePos = ezVec3(0);
    protected float m_fWanderRadius = 150.0f;
    protected float m_fLife = 10.0f;

    // Last navigation destination set via SetNavDestination(), kept for debug visualization.
    protected ezVec3 m_vDestination = ezVec3(0);
    protected bool m_bHasDestination = false;

    void OnMsgDamage(ezMsgDamage@ msg)
    {
        m_fLife -= msg.Damage;

        if (m_fLife < 0.0f)
        {
            ezPrefabs::SpawnPrefab("{ c1165a59-f3ff-4abf-858c-9d358fb2359a }", GetOwner().GetGlobalTransform());
            GetWorld().DeleteObjectDelayed(GetOwner().GetHandle());
        }
    }

    /// If navigation ended up in the Failed state (SetDestination()/SetDestinationDirect() could
    /// not find a path to the target at all, e.g. it's unreachable), tries to recover by flying
    /// straight to the nearest valid cell nearby. Returns true if navigation was in the Failed
    /// state this frame (whether or not recovery succeeded) - the caller should skip its normal
    /// logic in that case and let recovery play out.
    ///
    /// Note: the navigation component's ground-truth path position can no longer end up stuck
    /// inside a blocked voxel at runtime (it never leaves the computed, voxel-clear path), so this
    /// is only reachable from genuine pathfinding failures now, not from "ran into an obstacle".
    bool TryRecoverFromFailedState(ezAiVoxelNavigationComponent@ navComp, ezVec3 vOwnPos)
    {
        if (navComp.GetState() != ezAiVoxelNavigationComponentState::Failed)
            return false;

        ezVec3 vSafePoint;
        if (navComp.GetValidCellNearby(vOwnPos, 5.0f, vSafePoint))
        {
            navComp.SetDestinationDirect(vSafePoint);
        }

        return true;
    }

    /// Sets the navigation destination and records it (m_vDestination / m_bHasDestination) so it
    /// can be visualized.
    void SetNavDestination(ezAiVoxelNavigationComponent@ navComp, ezVec3 vPoint)
    {
        navComp.SetDestination(vPoint);
        m_vDestination = vPoint;
        m_bHasDestination = true;
    }

    /// Picks a random navigable point within m_fWanderRadius of the home position and heads there.
    void PickWanderDestination(ezAiVoxelNavigationComponent@ navComp)
    {
        ezVec3 vPoint;

        if (navComp.FindRandomPointAroundSphere(m_vHomePos, m_fWanderRadius, 32, vPoint))
        {
            SetNavDestination(navComp, vPoint);
        }
    }
}
