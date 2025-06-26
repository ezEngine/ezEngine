class ScriptObject : ezAngelScriptClass
{
    bool RadialShatter = true;
    int MaxImpacts = 10;
    private int Counter = 0;

    void OnImpact(ezVec3 pos, ezVec3 dir, bool radial, float size)
    {
        ++Counter;

        ezJoltBreakableSlabComponent@ slab;
        if (GetOwner().TryGetComponentOfBaseType(@slab))
        {
            slab.ContactReportForceThreshold = 5;
    
            if (Counter < MaxImpacts)
            {
                if (RadialShatter && radial)
                    slab.ShatterRadial(pos, size, dir, 0.5);
                else
                    slab.ShatterCellular(pos, size, dir, 1.0);
            }
            else
            {
                slab.ShatterAll(0.5, dir * 0.5);
            }
        }
    }
    
    void OnMsgDamage(ezMsgDamage@ msg) 
    {
        OnImpact(msg.GlobalPosition, msg.ImpactDirection * msg.Damage * 0.5, true, 0.15);
     }

     void OnMsgPhysicContact(ezMsgPhysicContact@ msg)
     {
        OnImpact(msg.GlobalPosition, msg.Normal * ezMath::Sqrt(msg.ImpactSqr), true, 0.4);
     }

     void OnMsgPhysicCharacterContact(ezMsgPhysicCharacterContact@ msg)
     {
        OnImpact(msg.GlobalPosition, msg.Normal * msg.Impact, false, 0.4);
     }
}
