enum State
{
    Idle,
    Active,
    Searching,
}

class NPC : ezAngelScriptClass
{
    int Health = 100;

    private State m_State = State::Idle;
    private ezVec3 m_vKnownPlayerLoc;
    private ezVec3 m_vLastFootstep;

    void OnSimulationStarted()
    {
        SetUpdateInterval(ezTime::Milliseconds(400));
        m_vLastFootstep = GetOwner().GetGlobalPosition();
    }

    void Shoot()
    {
        ezGameObject@ gunObj = GetOwner().FindChildByName("Gun");

        if (@gunObj == null)
            return;

        ezSpawnComponent@ spawnComp;
        if (!gunObj.TryGetComponentOfBaseType(@spawnComp))
            return;

        if (spawnComp.TriggerManualSpawn(false, ezVec3::MakeZero()))
        {
            // shot fired

            ezSound::PlaySound("{ d4e58c28-450f-449a-a920-7e0459066700 }", gunObj.GetGlobalPosition(), ezQuat::MakeIdentity(), 1, 1, true);
        }
    }

    void Update(ezTime deltaTime)
    {
        ezAiNavigationComponent@ navComp;
        if (!GetOwner().TryGetComponentOfBaseType(@navComp))
            return;

        const float fPlayDistance = 20.0f;

        // we could check this once (with a huge radius) and just store the result
        ezGameObject@ playerMarkerObj = ezSpatial::FindClosestObjectInSphere("Player", GetOwner().GetGlobalPosition(), fPlayDistance);
        if (@playerMarkerObj == null)
            return;

        ezVec3 vHitPosition, vitNormal;
        ezGameObjectHandle hHitObject;
        ezVec3 vStart = GetOwner().GetGlobalPosition() + ezVec3(0, 0, 1.5);
        ezVec3 vEnd = playerMarkerObj.GetGlobalPosition();

        if ((vEnd - vStart).GetLengthSquared() < (fPlayDistance * fPlayDistance))
        {
            if (!ezPhysics::OverlapTestLine(vStart, vEnd, 0, ezPhysicsShapeType(ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic)))
            {
                m_vKnownPlayerLoc = playerMarkerObj.GetGlobalPosition();
                m_State = State::Active;
                SetUpdateInterval(ezTime::Milliseconds(40));

                // ezDebug::DrawLine(vStart, vEnd, ezColor::IndianRed, ezColor::IndianRed);

                const float fDist = (vEnd - vStart).GetLength();
                navComp.StopWalking(ezMath::Min(1.0f, fDist * 0.3f));
                
                if (navComp.GetState() == ezAiNavigationComponentState::Idle)
                {
                    Shoot();
                }

                navComp.TurnTowards(vEnd.GetAsVec2());
            }
            else
            {
                // ezDebug::DrawLine(vStart, vEnd, ezColor::Yellow, ezColor::Yellow);

                if (m_State == State::Active)
                {
                    m_State = State::Searching;
                }
                else
                {
                    switch (navComp.GetState())
                    {
                    case ezAiNavigationComponentState::Idle:
                        m_State = State::Idle;
                        break;
                    }
                }
            }
        }

        if (m_State == State::Idle)
        {
            SetUpdateInterval(ezTime::Milliseconds(400));
            return;
        }

        if (m_State == State::Searching)
        {
            navComp.SetDestination(m_vKnownPlayerLoc, false);
        }

        if ((GetOwner().GetGlobalPosition() - m_vLastFootstep).GetLengthSquared() > 1.5f)
        {
            // walked more than one meter? spawn a footstep sound
            m_vLastFootstep = GetOwner().GetGlobalPosition();

            ezPhysics::RaycastSurfaceInteraction(m_vLastFootstep + ezVec3(0, 0, 0.1f), ezVec3(0, 0, -0.5f), 0, ezPhysicsShapeType::Static, "{ 0d634746-1154-4917-af6a-2d740fc752f5 }", "Footstep");
        }
    }

    void OnMsgDamage(ezMsgDamage@ msg)
    {
        if (Health <= 0)
            return;

        Health -= int(msg.Damage);

        if (Health <= 0)
        {
            ezTransform pos = GetOwner().GetGlobalTransform();
            pos.m_vPosition += ezVec3(0, 0, 0.9f);
            ezPrefabs::SpawnPrefab("{ 3d758985-7126-92f0-1cc1-36dcd5419ee1 }", pos);

            ezMsgDeleteGameObject del;
            GetOwner().PostMessage(del, ezTime::Milliseconds(100));
        }
    }
}