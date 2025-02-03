class ScriptObject :  ezAngelScriptClass
{
    private bool ragdollFinished = false;

    void OnMsgDamage(ezMsgDamage@ msg)
    {
        if (ragdollFinished)
            return;
            
        ezJoltHitboxComponent@ col;
        if (GetOwner().TryGetComponentOfBaseType(col))
        {
            // if present, deactivate the bone collider component, it isn't needed anymore
            col.Active = false;
        }
        
        ezJoltDynamicActorComponent@ da;
        if (GetOwner().TryGetComponentOfBaseType(da))
        {
            // if present, deactivate the dynamic actor component, it isn't needed anymore
            da.Active = false;
        }            
        
        ezJoltRagdollComponent@ rdc;
        if (GetOwner().TryGetComponentOfBaseType(rdc))
        {
            if (rdc.IsActiveAndSimulating())
            {
                ragdollFinished = true;
                return;
            }

            rdc.StartMode = ezJoltRagdollStartMode::WithCurrentMeshPose;
            rdc.Active = true;

            // we want the ragdoll to get a kick, so send an impulse message
            ezMsgPhysicsAddImpulse imp;
            imp.Impulse = msg.ImpactDirection;
            imp.Impulse *= ezMath::Min(msg.Damage, 5) * 10;
            imp.GlobalPosition = msg.GlobalPosition;
            rdc.SendMessage(imp);
        }
    }
}

