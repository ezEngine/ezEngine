class ScriptObject : ezAngelScriptClass
{
    int Health = 30;
    private bool HasTargetPosition = false;
    
    void OnSimulationStarted()
    {

    }
        
    void Update() 
    {
        if (Health <= 0)
            return;

        if (!HasTargetPosition)
        {
            // if the bot doesn't have a target yet, get a random one
            ezAiNavigationComponent@ navComp;
            if (GetOwner().TryGetComponentOfBaseType(@navComp))
            {
                ezVec3 point;
                if (navComp.FindRandomPointAroundCircle(ezVec3::MakeZero(), 10.0f, point))
                {
                    // FindRandomPointAroundCircle will fail, as long as the navmesh isn't loaded around that point
                    navComp.SetDestination(point, false);
                    HasTargetPosition = true;
                }
            }
        }

        ezLocalBlackboardComponent@ bbComp;
        if (GetOwner().TryGetComponentOfBaseType(@bbComp))
        {
            const float fAnimSpeed = 3.0f;

            // tell the walk animation how fast to play
            ezVec3 vVelocity = GetOwner().GetLinearVelocity();
            vVelocity.z = 0.0f;
            float fSpeed = vVelocity.GetLength();
            bbComp.SetEntryValue("MoveForwards", ezMath::Clamp(fSpeed / fAnimSpeed, 0.0f, 1.0f));
        }

        if (false)
        {
            // example for how to raycast the navmesh, to detect obstacles
            ezAiNavigationComponent@ navComp;
            if (GetOwner().TryGetComponentOfBaseType(@navComp))
            {
                ezVec3 vHit;
                float fHit;
                if (navComp.RaycastNavMesh(GetOwner().GetGlobalPosition(), GetOwner().GetGlobalDirForwards(), 10.0f, vHit, fHit))
                {
                    ezDebug::DrawLine(GetOwner().GetGlobalPosition()+ ezVec3(0, 0, 1), vHit + ezVec3(0, 0, 1), ezColor::Orange, ezColor::Red);
                }
            }
        }
    }
    
    void OnMsgDamage(ezMsgDamage@ msg) 
    {
        if (Health <= 0)
            return;

        Health -= int(msg.Damage);

        ezLocalBlackboardComponent@ bbComp;
        if (!GetOwner().TryGetComponentOfBaseType(@bbComp))
            return;

        // play a random hit reaction
        // the animation graph takes care of only playing one of those at the same time
        // and this won't interrupt a playing one
        int iHitReaction = GetWorld().GetRandomNumberGenerator().IntMinMax(0, 1);
        bbComp.SetEntryValue("PlayHitReaction", iHitReaction);

        if (Health <= 0)
        {
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

            // disable the navigation component, so that it doesn't update the position any further
            ezAiNavigationComponent@ navComp;
            if (GetOwner().TryGetComponentOfBaseType(@navComp))
            {
                navComp.Active = false;
            }
        }
    }

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
    }
}
