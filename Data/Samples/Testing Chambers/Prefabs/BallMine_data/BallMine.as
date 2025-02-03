enum BallMineState
{
    Init,
    Idle,
    Alert,
    Approaching,
    Attacking
}

class ScriptObject :  ezAngelScriptClass
{
    float AlertDistance = 15;
    float ApproachDistance = 10;
    float AttackDistance = 1.5;
    float RollForce = 200;
    float Health = 20;

    private ezGameObjectHandle _player;
    private BallMineState _state = BallMineState::Init;

    void OnSimulationStarted()
    {
        ezGameObject@ obj;
        if (GetWorld().TryGetObjectWithGlobalKey("Player", obj))
        {
            _player = obj.GetHandle();
        }

        Update();
    }

    // QueryForNPC = (go: ez.GameObject): boolean => {

    //     // just accept the first object that was found
    //     _player = go;
    //     return false;
    // }

    void Update()
    {
        auto oldState = _state;
        auto owner = GetOwner();

        if (_player.IsInvalidated())
        {
            // ez.World.FindObjectsInSphere("Player", owner.GetGlobalPosition(), AlertDistance, QueryForNPC);
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
                _state = BallMineState::Approaching;

                ezJoltDynamicActorComponent@ actor;
                if (GetOwner().TryGetComponentOfBaseType(actor))
                {
                    diffPos.Normalize();
                    diffPos *= RollForce;

                    actor.AddLinearForce(diffPos);
                }
            }
            else if (distToPlayer <= AlertDistance)
            {
                _state = BallMineState::Alert;
            }
            else
            {
                _state = BallMineState::Idle;
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

                    SetUpdateInterval(ezTime::MakeFromMilliseconds(500));
                    return;
                }
            case BallMineState::Alert:
                {
                    ezMsgSetMeshMaterial matMsg;
                    matMsg.Material = "{ 6ae73fcf-e09c-1c3f-54a8-8a80498519fb }";
                    GetOwner().SendMessageRecursive(matMsg);

                    SetUpdateInterval(ezTime::MakeFromMilliseconds(100));
                    return;
                }
            case BallMineState::Approaching:
                {
                    ezMsgSetMeshMaterial matMsg;
                    matMsg.Material = "{ 49324140-a093-4a75-9c6c-efde65a39fc4 }";
                    GetOwner().SendMessageRecursive(matMsg);

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
        if (GetOwner().TryGetComponentOfBaseType(spawnExpl))
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

