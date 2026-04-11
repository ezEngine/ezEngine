enum HunterState
{
    Wandering, // no prey in range, exploring randomly
    Pursuing,  // prey detected, navigating towards it
    Caught,    // reached the prey
}

class Hunter : ezAngelScriptClass
{
    int Difficulty = 3;

    bool ShowDebugInfo = true;

    ezString PreyMarker = "Prey";

    private HunterState m_State = HunterState::Wandering;
    private ezVec3 m_vLastKnownPreyPos;
    private bool m_bHasLastKnownPos = false;
    private ezTime m_CaughtResetTime;

    private float m_fSpeed = 5.0f;
    private float m_fDetectionRange = 20.0f;
    private float m_fCatchDistance = 1.8f;
    private float m_fWanderRadius = 8.0f;

    void OnSimulationStarted()
    {
        GetOwner().MakeDynamic();
        ApplyDifficulty();
    }

    void ApplyDifficulty()
    {
        if (Difficulty <= 1) // Easy
        {
            m_fSpeed          = 2.5f;
            m_fDetectionRange = 10.0f;
            m_fCatchDistance  = 2.0f;
            SetUpdateInterval(ezTime::Milliseconds(900));
        }
        else if (Difficulty == 2) // Medium
        {
            m_fSpeed          = 5.0f;
            m_fDetectionRange = 22.0f;
            m_fCatchDistance  = 1.8f;
            SetUpdateInterval(ezTime::Milliseconds(350));
        }
        else // Hard
        {
            m_fSpeed          = 9.5f;
            m_fDetectionRange = 40.0f;
            m_fCatchDistance  = 1.5f;
            SetUpdateInterval(ezTime::Milliseconds(60));
        }
    }

    void Update(ezTime deltaTime)
    {
        ezAiVoxelNavigationComponent@ navComp;
        if (!GetOwner().TryGetComponentOfBaseType(@navComp))
            return;

        navComp.Speed = m_fSpeed;

        ezVec3 vOwnPos    = GetOwner().GetGlobalPosition();
        ezVec3 vStatusPos = vOwnPos + ezVec3(0, 0, 2.2f);

        if (m_State == HunterState::Caught)
        {
            navComp.CancelNavigation();
            if (ShowDebugInfo)
                ezDebug::Draw3DText("Hunter: CAUGHT!", vStatusPos, ezColor::Red, 36);
            if (GetWorld().GetClock().GetAccumulatedTime() >= m_CaughtResetTime)
            {
                m_State = HunterState::Wandering;
                m_bHasLastKnownPos = false;
            }
            return;
        }

        ezGameObject@ preyObj = ezSpatial::FindClosestObjectInSphere(PreyMarker, vOwnPos, m_fDetectionRange);

        if (@preyObj != null)
        {
            m_vLastKnownPreyPos = preyObj.GetGlobalPosition();
            m_bHasLastKnownPos  = true;

            float fDist = vOwnPos.GetDistanceTo(m_vLastKnownPreyPos);

            if (fDist <= m_fCatchDistance)
            {
                m_State = HunterState::Caught;
                m_CaughtResetTime = GetWorld().GetClock().GetAccumulatedTime() + ezTime::Seconds(2.0);
                navComp.CancelNavigation();
            }
            else
            {
                m_State = HunterState::Pursuing;
                navComp.SetDestination(m_vLastKnownPreyPos);

                if (ShowDebugInfo)
                    ezDebug::DrawLine(vOwnPos, m_vLastKnownPreyPos, ezColor::OrangeRed, ezColor::Orange);
            }
        }
        else if (m_bHasLastKnownPos)
        {
            m_State = HunterState::Pursuing;
            navComp.SetDestination(m_vLastKnownPreyPos);

            float fDist = vOwnPos.GetDistanceTo(m_vLastKnownPreyPos);
            if (fDist <= m_fCatchDistance + 0.5f)
            {
                m_bHasLastKnownPos = false;
                m_State = HunterState::Wandering;
                PickWanderDestination(navComp);
            }
        }
        else
        {
            m_State = HunterState::Wandering;

            if (navComp.GetState() == ezAiVoxelNavigationComponentState::Idle ||
                navComp.GetState() == ezAiVoxelNavigationComponentState::Failed)
            {
                PickWanderDestination(navComp);
            }
        }

        if (ShowDebugInfo)
            DrawDebugState(vStatusPos, vOwnPos);
    }

    void PickWanderDestination(ezAiVoxelNavigationComponent@ navComp)
    {
        ezRandom@ rng = GetWorld().GetRandomNumberGenerator();
        ezVec3 vOwnPos = GetOwner().GetGlobalPosition();

        float rx = rng.FloatMinMax(-m_fWanderRadius, m_fWanderRadius);
        float ry = rng.FloatMinMax(-m_fWanderRadius, m_fWanderRadius);
        float rz = rng.FloatMinMax(-m_fWanderRadius * 0.5f, m_fWanderRadius * 0.5f);

        navComp.SetDestination(vOwnPos + ezVec3(rx, ry, rz));
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
        else
        {
            ezDebug::Draw3DText("Hunter: CAUGHT!", vTextPos, ezColor::Red, 36);
        }

        ezDebug::DrawLineSphere(vOwnPos, m_fDetectionRange, ezColor(1, 0.3f, 0, 0.3f));
    }
}
