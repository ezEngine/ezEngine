class MsgRandomAnimTick : ezAngelScriptMessage {}

class ScriptObject : ezAngelScriptClass
{
    void OnSimulationStarted()
    {
        MsgRandomAnimTick tick;
        GetOwner().PostMessage(tick, ezTime::MakeFromSeconds(1.0));
    }

    void OnMsgRandomAnimTick(MsgRandomAnimTick@ msg)
    {
        ezLocalBlackboardComponent@ bbComp;
        if (GetOwner().TryGetComponentOfBaseType(@bbComp))
        {
            ezRandom@ rng = GetWorld().GetRandomNumberGenerator();
            ezUInt32 pick = rng.UIntInRange(6);

            bbComp.SetEntryValue("IsAlive", true);
            bbComp.SetEntryValue("Shoot", -1);

            if (pick == 0)
                bbComp.SetEntryValue("MoveForwards", float(rng.UIntInRange(2)));
            else if (pick == 1)
                bbComp.SetEntryValue("IsAlive", rng.UIntInRange(2) == 0);
            else if (pick == 2)
                bbComp.SetEntryValue("PlayHitReaction", int(rng.UIntInRange(2)));
            else if (pick == 3)
                bbComp.SetEntryValue("Shoot", rng.IntMinMax(-1, 1));
            else if (pick == 4)
                bbComp.SetEntryValue("PlayIdleAction", int(rng.UIntInRange(2)));
            else
                bbComp.SetEntryValue("Melee", int(rng.UIntInRange(2)));
        }

        MsgRandomAnimTick nextTick;
        GetOwner().PostMessage(nextTick, ezTime::MakeFromSeconds(1.0));
    }
}
