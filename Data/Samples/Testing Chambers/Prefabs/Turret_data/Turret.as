class ScriptObject : ezAngelScriptClass
{
    float Health = 50;
    private uint8 CollisionLayer = 0;

    ezGameObjectHandle target;
    ezComponentHandle gunSpawn;
    ezComponentHandle gunSound;

    void OnMsgDamage(ezMsgDamage@ msg)
    {
        if (Health <= 0)
            return;

        Health -= msg.Damage;

        if (Health > 0)
            return;

        SetUpdateInterval(ezTime::Seconds(60)); // basically deactivate future updates

        auto expObj = GetOwner().FindChildByName("Explosion", true);
        if (@expObj == null)
            return;

        ezSpawnComponent@ spawnComp;
        if (expObj.TryGetComponentOfBaseType(@spawnComp))
        {
            spawnComp.TriggerManualSpawn(true, ezVec3::MakeZero());
        }
    }

    void OnSimulationStarted()
    {
        SetUpdateInterval(ezTime::Milliseconds(500));

        CollisionLayer = ezPhysics::GetCollisionLayerByName("Visibility Raycast");

        auto gunObj = GetOwner().FindChildByName("Gun", true);

        ezSpawnComponent@ gunSpawnComp;
        if (gunObj.TryGetComponentOfBaseType(@gunSpawnComp))
            gunSpawn = gunSpawnComp.GetHandle();

        ezFmodEventComponent@ gunSoundComp;
        if (gunObj.TryGetComponentOfBaseType(@gunSoundComp))
            gunSound = gunSoundComp.GetHandle();
    }

    bool FoundObjectCallback(ezGameObject@ go)
    {
        target = go.GetHandle();
        return false;
    }

    void Update(ezTime deltaTime)
    {
        if (Health <= 0)
            return;

        if (gunSpawn.IsInvalidated())
            return;

        ezGameObject@ owner = GetOwner();

        target.Invalidate();
        ezSpatial::FindObjectsInSphere("Player", owner.GetGlobalPosition(), 15, ReportObjectCB(FoundObjectCallback));

        ezGameObject@ targetObj;
        if (!GetWorld().TryGetObject(target, @targetObj))
        {
            SetUpdateInterval(ezTime::Milliseconds(500));
            return;
        }

        ezVec3 dirToTarget = targetObj.GetGlobalPosition() - owner.GetGlobalPosition();

        const float distance = dirToTarget.GetLengthAndNormalize();

        ezVec3 vHitPosition;
        ezVec3 vHitNormal;
        ezGameObjectHandle HitObject;

        if (ezPhysics::Raycast(vHitPosition, vHitNormal, HitObject, owner.GetGlobalPosition(), dirToTarget * distance, CollisionLayer, ezPhysicsShapeType::Static))
        {
            // obstacle in the way
            return;
        }

        SetUpdateInterval(ezTime::Milliseconds(50));

        ezQuat targetRotation = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), dirToTarget);

        ezQuat newRotation = ezQuat::MakeSlerp(owner.GetGlobalRotation(), targetRotation, 0.1);

        owner.SetGlobalRotation(newRotation);

        dirToTarget.Normalize();

        if (dirToTarget.Dot(owner.GetGlobalDirForwards()) > ezMath::Cos(ezAngle::MakeFromDegree(15)))
        {
            ezSpawnComponent@ gunSpawnComp;
            if (GetWorld().TryGetComponent(gunSpawn, @gunSpawnComp))
            {
                gunSpawnComp.ScheduleSpawn();
            }

            ezFmodEventComponent@ gunSoundComp;
            if (GetWorld().TryGetComponent(gunSound, @gunSoundComp))
            {
                gunSoundComp.StartOneShot();
            }
        }
    }
}

