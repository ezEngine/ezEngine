class TargetSphere : ezAngelScriptClass
{
    private int curDamage = 0;

    void OnSimulationStarted()
    {
        SetUpdateInterval(ezTime::Milliseconds(100));
    }

    void OnMsgDamage(ezMsgDamage@ msg)
    {
        curDamage += int(msg.Damage);
    }

    void OnMsgInputActionTriggered(ezMsgInputActionTriggered@ msg)
    {
        if (msg.TriggerState == ezTriggerState::Activated)
        {
            if (msg.InputAction == "Heal")
            {
                curDamage = 0;
            }
        }
    }

    void Update()
    {
        curDamage = ezMath::Clamp(curDamage - 1, 0, 1000.0f);
        float dmg = curDamage / 100.0f;

        ezMsgSetColor msgCol;
        msgCol.Color = ezColor::MakeRGBA(dmg, dmg * 0.05, dmg * 0.05, 1.0);

        GetOwner().SendMessageRecursive(msgCol);

        ezParticleComponent@ fireFX;
        if (GetOwner().TryGetComponentOfBaseType(@fireFX))
        {
            if (dmg > 1.0)
            {
                if (!fireFX.IsEffectActive())
                {
                    fireFX.StartEffect();
                }
            }
            else if (dmg < 0.8)
            {
                fireFX.StopEffect();
            }
        }
    }
}

