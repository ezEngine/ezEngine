enum BallMineState
{
    Init,
    Idle,
    Alert,
    Approaching,
    Attacking
}

class ScriptObject : ezAngelScriptClass
{
    float AlertDistance = 15;
    float ApproachDistance = 10;
    float AttackDistance = 1.5;
    float RollForce = 40;
    float Health = 20;
    
    private ezGameObjectHandle _player;
    private BallMineState _state = BallMineState::Init;
    private uint32 _forceID = 0;
 
    void OnSimulationStarted()
    {
        ezGameObject@ obj;
        if (GetWorld().TryGetObjectWithGlobalKey("Player", obj))
        {
            _player = obj.GetHandle();
        }

        Update(ezTime::MakeZero());
    }

    bool QueryForNPC(ezGameObject@ go)
    {
         // just accept the first object that was found
         _player = go.GetHandle();
         return false;
    }

    void SetLight(bool on, ezColor color)
    {
        ezLightComponent@ light;
        if (!GetOwner().TryGetComponentOfBaseType(@light))
            return;

        light.Active = on;
        light.LightColor = color;
    }

    void Update(ezTime deltaTime)
    {
        auto oldState = _state;
        auto owner = GetOwner();

        if (_player.IsInvalidated())
        {
            ezSpatial::FindObjectsInSphere("Player", owner.GetGlobalPosition(), AlertDistance, ReportObjectCB(QueryForNPC));
            return;
        }

        ezGameObject@ playerObj;
        if (GetWorld().TryGetObject(_player, playerObj))
        {
            auto playerPos = playerObj.GetGlobalPosition();
            auto ownPos = GetOwner().GetGlobalPosition();
            auto diffPos = playerPos - ownPos;
            auto distToPlayer = diffPos.GetLength();

            // ezLog::Info("Distance to Player: {}", distToPlayer);

            if (distToPlayer <= ApproachDistance) 
            {
                if (_state == BallMineState::Alert)
                {
                    ezSound::PlaySound("{ 35b72527-bc85-4b33-802a-0fba75cf9acb }", GetOwner().GetGlobalPosition(), ezQuat::MakeIdentity(), 1.0f, 1.0f, false);
                }

                _state = BallMineState::Approaching;

                ezJoltDynamicActorComponent@ actor;
                if (GetOwner().TryGetComponentOfBaseType(@actor))
                {
                    diffPos.Normalize();
                    diffPos *= RollForce;

                    _forceID = actor.AddOrUpdateForce(_forceID, ezTime::Seconds(0.5), diffPos);
                }
            }
            else if (distToPlayer <= AlertDistance)
            {
                if (_state == BallMineState::Idle)
                {
                    ezSound::PlaySound("{ 497a67cc-1939-4fde-840f-df9b83d8205a }", GetOwner().GetGlobalPosition(), ezQuat::MakeIdentity(), 1.0f, 1.0f, false);
                }

                _state = BallMineState::Alert;
            }
            else
            {
                if (_state != BallMineState::Idle)
                {
                    _state = BallMineState::Idle;
                    ezSound::PlaySound("{ ae6ab36e-94ab-4fa9-b591-3e6a2851d6db }", GetOwner().GetGlobalPosition(), ezQuat::MakeIdentity(), 1.0f, 1.0f, false);
                }
            }

            if (distToPlayer <= AttackDistance)
            {
                _state = BallMineState::Attacking;
            }
        }
        else
        {
            _state = BallMineState::Idle;
            _player.Invalidate();
        }

        if (oldState != _state)
        {
            switch (_state)
            {
            case BallMineState::Idle:
                {
                    ezMsgSetMeshMaterial matMsg;
                    matMsg.Material = "{ d615cd66-0904-00ca-81f9-768ff4fc24ee }";
                    GetOwner().SendMessageRecursive(matMsg);

                    SetLight(false, ezColor::MakeZero());
                    SetUpdateInterval(ezTime::MakeFromMilliseconds(1000));
                    return;
                }
            case BallMineState::Alert:
                {
                    ezMsgSetMeshMaterial matMsg;
                    matMsg.Material = "{ 6ae73fcf-e09c-1c3f-54a8-8a80498519fb }";
                    GetOwner().SendMessageRecursive(matMsg);

                    SetLight(true, ezColor(1, 0.5f, 0));
                    SetUpdateInterval(ezTime::MakeFromMilliseconds(500));
                    return;
                }
            case BallMineState::Approaching:
                {
                    ezMsgSetMeshMaterial matMsg;
                    matMsg.Material = "{ 49324140-a093-4a75-9c6c-efde65a39fc4 }";
                    GetOwner().SendMessageRecursive(matMsg);

                    SetLight(true, ezColor(1, 0, 0));
                    SetUpdateInterval(ezTime::MakeFromMilliseconds(50));
                    return;
                }
            case BallMineState::Attacking:
                {
                    Explode();
                    return;
                }
            }
        }
    }

    void Explode()
    {
        ezSpawnComponent@ spawnExpl;
        if (GetOwner().TryGetComponentOfBaseType(@spawnExpl))
        {
            spawnExpl.TriggerManualSpawn(true, ezVec3::MakeZero());
        }

        GetWorld().DeleteObjectDelayed(GetOwner().GetHandle());
    }

    void OnMsgDamage(ezMsgDamage@ msg)
    {
        if (Health > 0) 
        {
            Health -= msg.Damage;

            if (Health <= 0)
            {
                Explode();
            }
        }
    }
}
