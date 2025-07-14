class ScriptObject : ezAngelScriptClass
{
    int Health = 30;
    private bool HasTargetPosition = false;
    
    void OnSimulationStarted()
    {

    }
        
    void Update() 
    {
        if (!HasTargetPosition)
        {
            ezAiNavigationComponent@ navComp;
            if (GetOwner().TryGetComponentOfBaseType(@navComp))
            {
                ezVec3 point;
                if (navComp.FindRandomPointAroundCircle(ezVec3::MakeZero(), 10.0f, point))
                {
                    navComp.SetDestination(point, false);
                    HasTargetPosition = true;
                }
            }
        }

        ezSimpleAnimationComponent@ animComp;
        if (GetOwner().TryGetComponentOfBaseType(@animComp))
        {
            const float fAnimSpeed = 5.0f;

            ezVec3 vVelocity = GetOwner().GetLinearVelocity();
            vVelocity.z = 0.0f;
            float fSpeed = vVelocity.GetLength();
            animComp.Speed = ezMath::Clamp(fSpeed / fAnimSpeed, 0.0f, 1.0f);
        }

        if (Health <= 0)
            return;

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
    
    void OnMsgDamage(ezMsgDamage@ msg) 
    {
        if (Health <= 0)
            return;

        Health -= msg.Damage;

        if (Health <= 0)
        {
            ezJoltRagdollComponent@ ragComp;
            if (GetOwner().TryGetComponentOfBaseType(@ragComp))
            {
                ragComp.Active = true;
                ragComp.SetInitialImpulse(msg.GlobalPosition, msg.ImpactDirection * msg.Damage * 50);
            }

            ezAiNavigationComponent@ navComp;
            if (GetOwner().TryGetComponentOfBaseType(@navComp))
            {
                navComp.Active = false;
            }
        }
    }
}
