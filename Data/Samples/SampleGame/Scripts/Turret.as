class Turret : ezAngelScriptClass
{
    float Range = 3;

    private array<ezGameObjectHandle> allTargets;
    private ezTime lastDamageTime;

    void OnSimulationStarted()
    {
        // update this component every frame
        SetUpdateInterval(ezTime::MakeZero());
        lastDamageTime = GetWorld().GetClock().GetAccumulatedTime();
    }

    bool FoundTargetCallback(ezGameObject@ go)
    {
        allTargets.PushBack(go.GetHandle());
        return true;
    }

    void Update()
    {
        auto owner = GetOwner();

        // find all objects with the 'TurretTarget' marker that are close by
        allTargets.Clear();
        ezSpatial::FindObjectsInSphere("TurretTarget", owner.GetGlobalPosition(), Range, ReportObjectCB(FoundTargetCallback));

        DrawLinesToTargets();

        const ezTime gameTime = GetWorld().GetClock().GetAccumulatedTime();

        if (gameTime - lastDamageTime > ezTime::Milliseconds(40))
        {
            lastDamageTime = gameTime;
            DamageAllTargets(4);
        }
    }

    void DrawLinesToTargets()
    {
        ezVec3 startPos = GetOwner().GetGlobalPosition();

        for (uint i = 0; i < allTargets.GetCount(); ++i)
        {
            ezGameObject@ obj;
            if (GetWorld().TryGetObject(allTargets[i], obj))
            {
                ezVec3 endPos = obj.GetGlobalPosition();
                ezDebug::DrawLine(startPos, endPos, ezColor::OrangeRed, ezColor::OrangeRed);
            }
        }

    }

    void DamageAllTargets(float damage)
    {
        ezMsgDamage dmgMsg;
        dmgMsg.Damage = damage;

        for (uint i = 0; i < allTargets.GetCount(); ++i)
        {
            GetWorld().SendMessage(allTargets[i], dmgMsg);
        }
    }
}

