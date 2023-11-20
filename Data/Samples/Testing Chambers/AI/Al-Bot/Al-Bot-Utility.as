// Character logic for the utility AI version of the Al bot.
// All decision making is done by the ezAiComponent (behavior set 'AlBot').
// This script only handles what isn't decision logic: health, death, ragdoll, animation events.

class Bot : ezAngelScriptClass
{
    int Health = 50;
    private bool _isDead = false;

    // the 'ShotNoise' marker is active until this time, other NPCs can hear it
    private ezTime _shotNoiseEnd;

    void SetShotNoiseActive(bool active)
    {
        ezGameObject@ noiseObj = GetOwner().FindChildByName("ShotNoise");
        if (@noiseObj == null)
            return;

        ezAiMarkerComponent@ markerComp;
        if (noiseObj.TryGetComponentOfBaseType(@markerComp))
        {
            markerComp.Active = active;
        }
    }

    void DeactivateGO(ezStringView object)
    {
        ezGameObject@ obj = GetOwner().FindChildByName(object, true);
        if (@obj != null)
        {
            obj.SetActiveFlag(false);
        }
    }

    void DeactivateAll()
    {
        DeactivateGO("OnDeathDeactivate");
        UpdateFootstepNoise(false, false);

        // stop making decisions
        ezAiComponent@ aiComp;
        if (GetOwner().TryGetComponentOfBaseType(@aiComp))
        {
            aiComp.Active = false;
        }

        // disable the navigation component, so that it doesn't update the position any further
        ezAiNavigationComponent@ navComp;
        if (GetOwner().TryGetComponentOfBaseType(@navComp))
        {
            navComp.Active = false;
        }

        ezJoltHitboxComponent@ hitComp;
        if (GetOwner().TryGetComponentOfBaseType(@hitComp))
        {
            hitComp.Active = false;
        }
    }

    void SetNoiseMarkerActive(ezStringView name, bool active)
    {
        ezGameObject@ noiseObj = GetOwner().FindChildByName(name);
        if (@noiseObj == null)
            return;

        ezAiMarkerComponent@ markerComp;
        if (noiseObj.TryGetComponentOfBaseType(@markerComp))
        {
            markerComp.Active = active;
        }
    }

    // other NPCs can hear the 'FootstepWalk' / 'FootstepRun' markers, their radius defines how far
    void UpdateFootstepNoise(bool moving, bool running)
    {
        SetNoiseMarkerActive("FootstepWalk", moving && !running);
        SetNoiseMarkerActive("FootstepRun", moving && running);
    }

    void Update()
    {
        if (!_shotNoiseEnd.IsZero() && GetWorld().GetClock().GetAccumulatedTime() >= _shotNoiseEnd)
        {
            _shotNoiseEnd = ezTime::MakeZero();
            SetShotNoiseActive(false);
        }

        if (_isDead)
            return;

        ezLocalBlackboardComponent@ bbComp;
        if (!GetOwner().TryGetComponentOfBaseType(@bbComp))
            return;

        // tell the walk animation how fast to play
        const float fAnimSpeed = 3.0f;
        ezVec3 vVelocity = GetOwner().GetLinearVelocity();
        vVelocity.z = 0.0f;
        float fSpeed = vVelocity.GetLength();
        bbComp.SetEntryValue("MoveForwards", ezMath::Clamp(fSpeed / fAnimSpeed, 0.0f, 1.0f));

        UpdateFootstepNoise(fSpeed > 0.5f, fSpeed > 2.5f);
    }

    void OnMsgDamage(ezMsgDamage@ msg)
    {
        // the ezAiComponent receives this message as well and reacts to it

        if (_isDead)
            return;

        ezLocalBlackboardComponent@ bbComp;
        if (!GetOwner().TryGetComponentOfBaseType(@bbComp))
            return;

        Health -= int(msg.Damage);

        // read by the 'Health' consideration of the AI controller
        bbComp.SetEntryValue("Health", Health);

        // play a random hit reaction
        // the animation graph takes care of only playing one of those at the same time
        // and this won't interrupt a playing one
        int iHitReaction = GetWorld().GetRandomNumberGenerator().IntMinMax(0, 1);
        bbComp.SetEntryValue("PlayHitReaction", iHitReaction);

        if (Health <= 0)
        {
            _isDead = true;
            DeactivateAll();

            // let the animation graph know that the bot died, so that it can play the death animation
            bbComp.SetEntryValue("IsAlive", false);

            // also enable the ragdoll, if we have one
            ezJoltRagdollComponent@ ragComp;
            if (GetOwner().TryGetComponentOfBaseType(@ragComp))
            {
                // enable the ragdoll component to have it take over the animation process
                ragComp.Active = true;

                // fade out the death animation over a short time
                ragComp.FadeJointMotorStrength(0.0f, ezTime::MakeFromSeconds(1.6f));

                // add a start impulse, to push the ragdoll procedurally
                ragComp.SetInitialImpulse(msg.GlobalPosition, msg.ImpactDirection * msg.Damage * 50 * 10);
            }
        }
    }

    /////////////////////////////////////////////////////////////
    // Generic Events

    void OnMsgGenericEvent(ezMsgGenericEvent@ msg)
    {
        // if the playing (death) animation has a marker to switch to "powered" mode, forward that request to the ragdoll
        if (msg.Message == "AnimMode-Powered")
        {
            ezJoltRagdollComponent@ ragComp;
            if (GetOwner().TryGetComponentOfBaseType(@ragComp))
            {
                ragComp.AnimMode = ezJoltRagdollAnimMode::Powered;
            }

            return;
        }

        // if the playing (death) animation has a marker to switch to "limp" mode, forward that request to the ragdoll
        if (msg.Message == "AnimMode-Limp")
        {
            ezJoltRagdollComponent@ ragComp;
            if (GetOwner().TryGetComponentOfBaseType(@ragComp))
            {
                ragComp.AnimMode = ezJoltRagdollAnimMode::Limp;
            }

            return;
        }

        if (_isDead)
            return;

        if (msg.Message == "gun.shoot")
        {
            SetShotNoiseActive(true);
            _shotNoiseEnd = GetWorld().GetClock().GetAccumulatedTime() + ezTime::Seconds(1.5);

            ezGameObject@ muzzleObj = GetOwner().FindChildByName("Muzzle", true);
            if (@muzzleObj != null)
            {
                ezParticleComponent@ particleComp;
                if (muzzleObj.TryGetComponentOfBaseType(@particleComp))
                {
                    particleComp.StartEffect();
                }

                ezFmodEventComponent@ fmodComp;
                if (muzzleObj.TryGetComponentOfBaseType(@fmodComp))
                {
                    fmodComp.StartOneShot();
                }
            }

            ezGameObject@ spawnerObj = GetOwner().FindChildByName("BulletSpawner", true);
            if (@spawnerObj != null)
            {
                ezSpawnComponent@ spawnComp;
                if (spawnerObj.TryGetComponentOfBaseType(@spawnComp))
                {
                    spawnComp.TriggerManualSpawn(true, ezVec3::MakeZero());
                }
            }

            return;
        }

        if (msg.Message == "Melee-Hit")
        {
            ezGameObject@ spawnerObj = GetOwner().FindChildByName("Melee-Damage", true);
            if (@spawnerObj != null)
            {
                ezAreaDamageComponent@ dmgComp;
                if (spawnerObj.TryGetComponentOfBaseType(@dmgComp))
                {
                    dmgComp.ApplyAreaDamage();
                }
                ezSound::PlaySound("{ 32cb0079-08cc-4f6b-a94b-0832e602ee1a }", GetOwner().GetGlobalPosition() + ezVec3(0, 0, 1.5), ezQuat::MakeIdentity(), 1.0f, 1.0f, false);
            }
        }

        if (msg.Message == "walk.foot.l" || msg.Message == "walk.foot.r" || msg.Message == "run.foot.l" || msg.Message == "run.foot.r")
        {
            auto pos = GetOwner().GetGlobalPosition();
            ezPhysics::RaycastSurfaceInteraction(pos + ezVec3(0, 0, 0.1f), ezVec3(0, 0, -0.5f), 0, ezPhysicsShapeType::Static, "{ 0a105955-e069-4523-80ce-7b9867bde687 }", "Footstep");
        }
    }
}
