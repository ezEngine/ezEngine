class ScriptObject : ezAngelScriptClass
{
    int Health = 30;
    
    void OnSimulationStarted()
    {
        ezAiNavigationComponent@ navComp;
        if (GetOwner().TryGetComponentOfBaseType(@navComp))
        {
            navComp.SetDestination(ezVec3::MakeZero(), false);
        }
    }
        
    // void Update() { }
    
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
