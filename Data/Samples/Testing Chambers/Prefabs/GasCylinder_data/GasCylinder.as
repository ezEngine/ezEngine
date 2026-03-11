class ScriptObject :  ezAngelScriptClass
{
    private float capHealth = 5;
    private float bodyHealth = 50;
    private uint32 capForce = 0;

    void OnSimulationStarted()
    {
        // no update needed by default
        SetUpdateInterval(ezTime::MakeFromSeconds(10));
    }

    void Update(ezTime deltaTime)
    {
        if (capHealth <= 0) 
        {
            auto owner = GetOwner();
            auto cap = owner.FindChildByName("Cap");

            ezJoltDynamicActorComponent@ actor;
            if (owner.TryGetComponentOfBaseType(@actor))
            {
                ezVec3 force = cap.GetGlobalDirUp();

                auto randomDir = ezVec3::MakeRandomDirection(GetWorld().GetRandomNumberGenerator());
                randomDir *= 0.4;
    
                force += randomDir;
                force *= -2;

                actor.AddOrUpdateForce(capForce, ezTime::Seconds(0.5f), force);
            }
        }
    }

    void OnMsgDamage(ezMsgDamage@ msg)
    {
        bodyHealth -= msg.Damage;

        if (bodyHealth <= 0)
        {
            Explode();
            return;
        }

        if (msg.HitObjectName == "Cap")
        {
            if (capHealth > 0) 
            {
                capHealth -= msg.Damage;

                if (capHealth <= 0) 
                {
                    // update every frame, to apply regularly the physics force
                    SetUpdateInterval(ezTime::MakeZero());

                    auto leakObj = GetOwner().FindChildByName("LeakEffect");
                    if (@leakObj != null)
                    {
                        ezParticleComponent@ leakFX;
                        if (leakObj.TryGetComponentOfBaseType(@leakFX))
                        {
                            leakFX.StartEffect();
                        }
                        else
                        {
                            ezLog::Error("Failed to start particle effect!");
                        }

                        ezFmodEventComponent@ leakSound;
                        if (leakObj.TryGetComponentOfBaseType(@leakSound))
                        {
                            leakSound.Play();
                        }
                    }

                    // trigger code path below
                    msg.HitObjectName = "Tick";
                }
            }
        }

        if (msg.HitObjectName == "Tick")
        {
            ezMsgDamage tickDmg;
            tickDmg.Damage = 1;
            tickDmg.HitObjectName = "Tick";
            GetOwner().PostMessage(tickDmg, ezTime::MakeFromMilliseconds(100));
        }
    }

    void Explode()
    {
        auto owner = GetOwner();
        auto exp = owner.FindChildByName("Explosion");

        if (@exp != null)
        {
            ezSpawnComponent@ spawnExpl;
            if (exp.TryGetComponentOfBaseType(@spawnExpl))
            {
                spawnExpl.TriggerManualSpawn(false, ezVec3::MakeZero());
            }
        }

        GetWorld().DeleteObjectDelayed(GetOwner().GetHandle());
    }
}

