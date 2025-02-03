class ScriptObject :  ezAngelScriptClass
{
    private float distance = 0;

    void OnMsgGenericEvent(ezMsgGenericEvent@ msg)
    {
        if (msg.Message == "RaycastChanged")
        {
            ezLog::Info("RaycastChanged");

            ezGameObject@ beamObj = GetOwner().FindChildByName("Beam");

            ezRaycastComponent@ rayComp;
            if (beamObj.TryGetComponentOfBaseType(rayComp))
            {
                const float newDist = rayComp.GetCurrentDistance();
                if (newDist < distance - 0.01)
                {
                    // allow some slack
                    Explode();
                }

                distance = newDist;
            }
        }
    }

    void Explode()
    {
        ezGameObject@ exp = GetOwner().FindChildByName("Explosion");

        if (exp != null)
        {
            ezSpawnComponent@ spawnExpl;
            if (exp.TryGetComponentOfBaseType(spawnExpl))
            {
                spawnExpl.TriggerManualSpawn(true, ezVec3::MakeZero());
            }
        }

        GetWorld().DeleteObjectDelayed(GetOwner().GetHandle());
    }

    void OnMsgDamage(ezMsgDamage@ msg)
    {
        // explode on any damage
        Explode();
    }
}

