enum PreyState
{
    Wandering, // no threat nearby
    Fleeing,   // hunter detected, actively evading
    Cornered,  // nav failed while fleeing, trying an emergency escape
}

class Prey : ezAngelScriptClass
{
    int Difficulty = 3;

    bool ShowDebugInfo = true;

    ezString HunterMarker = "Hunter";

    private PreyState m_State = PreyState::Wandering;
    private ezGameObjectHandle m_hTrackedHunter;

    private float m_fSpeed = 5.5f;
    private float m_fAwarenessRange = 18.0f;
    private float m_fFleeDistance = 15.0f;   // how far to run when fleeing
    private float m_fPanicRange = 5.0f;      // emergency flee if hunter this close

    void OnSimulationStarted()
    {
        GetOwner().MakeDynamic();
        ApplyDifficulty();
    }

    void ApplyDifficulty()
    {
        if (Difficulty <= 1) // Easy
        {
            m_fSpeed          = 3.0f;
            m_fAwarenessRange = 8.0f;
            m_fFleeDistance   = 10.0f;
            m_fPanicRange     = 3.0f;
            SetUpdateInterval(ezTime::Milliseconds(700));
        }
        else if (Difficulty == 2) // Medium
        {
            m_fSpeed          = 5.5f;
            m_fAwarenessRange = 18.0f;
            m_fFleeDistance   = 15.0f;
            m_fPanicRange     = 5.0f;
            SetUpdateInterval(ezTime::Milliseconds(300));
        }
        else // Hard
        {
            m_fSpeed          = 8.5f;
            m_fAwarenessRange = 30.0f;
            m_fFleeDistance   = 22.0f;
            m_fPanicRange     = 8.0f;
            SetUpdateInterval(ezTime::Milliseconds(80));
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

        // Detect nearest hunter
        ezGameObject@ hunterObj = ezSpatial::FindClosestObjectInSphere(HunterMarker, vOwnPos, m_fAwarenessRange);

        if (@hunterObj != null)
        {
            ezVec3 vHunterPos = hunterObj.GetGlobalPosition();
            float fDistToHunter = vOwnPos.GetDistanceTo(vHunterPos);

            m_State = (fDistToHunter < m_fPanicRange) ? PreyState::Cornered : PreyState::Fleeing;

            if (navComp.GetState() == ezAiVoxelNavigationComponentState::Idle ||
                navComp.GetState() == ezAiVoxelNavigationComponentState::Failed)
            {
                if (fDistToHunter < m_fPanicRange)
                {
                    PickFleeDestinationRandom(navComp, vOwnPos);
                }
                else
                {
                    ezVec3 vAwayDir = vOwnPos - vHunterPos;
                    float fLen = vAwayDir.GetLength();

                    if (fLen > 0.01f)
                    {
                        vAwayDir = vAwayDir * (1.0f / fLen); // normalize manually

                        if (Difficulty >= 3)
                        {
                            ezRandom@ rng = GetWorld().GetRandomNumberGenerator();
                            float lateralBias = rng.FloatMinMax(-0.4f, 0.4f);
                            ezVec3 vSide = vAwayDir.GetOrthogonalVector();
                            vAwayDir = (vAwayDir + vSide * lateralBias).GetNormalized();
                        }

                        navComp.SetDestination(vOwnPos + vAwayDir * m_fFleeDistance);
                    }
                    else
                    {
                        PickFleeDestinationRandom(navComp, vOwnPos);
                    }
                }
            }

            if (ShowDebugInfo)
                ezDebug::DrawLine(vOwnPos, vHunterPos, ezColor::Yellow, ezColor::OrangeRed);
        }
        else
        {
            m_State = PreyState::Wandering;

            if (navComp.GetState() == ezAiVoxelNavigationComponentState::Idle ||
                navComp.GetState() == ezAiVoxelNavigationComponentState::Failed)
            {
                PickWanderDestination(navComp, vOwnPos);
            }
        }

        if (ShowDebugInfo)
            DrawDebugState(vStatusPos, vOwnPos);
    }

    void PickWanderDestination(ezAiVoxelNavigationComponent@ navComp, ezVec3 vOwnPos)
    {
        ezRandom@ rng = GetWorld().GetRandomNumberGenerator();
        float r = 9.0f;

        float rx = rng.FloatMinMax(-r, r);
        float ry = rng.FloatMinMax(-r, r);
        float rz = rng.FloatMinMax(-r * 0.4f, r * 0.4f);

        navComp.SetDestination(vOwnPos + ezVec3(rx, ry, rz));
    }

    void PickFleeDestinationRandom(ezAiVoxelNavigationComponent@ navComp, ezVec3 vOwnPos)
    {
        ezRandom@ rng = GetWorld().GetRandomNumberGenerator();
        float r = m_fFleeDistance;

        float rx = rng.FloatMinMax(-r, r);
        float ry = rng.FloatMinMax(-r, r);
        float rz = rng.FloatMinMax(-r * 0.4f, r * 0.4f);

        navComp.SetDestination(vOwnPos + ezVec3(rx, ry, rz));
    }

    void DrawDebugState(ezVec3 vTextPos, ezVec3 vOwnPos)
    {
        if (m_State == PreyState::Wandering)
        {
            ezDebug::Draw3DText("Prey: Wandering", vTextPos, ezColor::Green, 24);
        }
        else if (m_State == PreyState::Fleeing)
        {
            ezDebug::Draw3DText("Prey: FLEEING!", vTextPos, ezColor::Yellow, 28);
        }
        else
        {
            ezDebug::Draw3DText("Prey: Cornered!", vTextPos, ezColor::Orange, 28);
        }

        // Visualize awareness sphere
        ezDebug::DrawLineSphere(vOwnPos, m_fAwarenessRange, ezColor(0, 1, 0, 0.25f));
    }
}
