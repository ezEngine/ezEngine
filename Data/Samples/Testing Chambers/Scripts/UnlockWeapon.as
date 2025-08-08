#include "GameDecls.as"

class UnlockWeapon : ezAngelScriptClass
{
    int weaponType = 0;

    void OnMsgTriggerTriggered(ezMsgTriggerTriggered@ msg)
    {
        if (msg.TriggerState == ezTriggerState::Activated && msg.Message == "Pickup")
        {
            MsgUnlockWeapon hm;
            hm.weaponType = WeaponType(weaponType);

            GetWorld().SendMessageRecursive(msg.GameObject, hm);

            if (!hm.return_consumed)
                return;

            ezFmodEventComponent@ sound;
            if (GetOwner().TryGetComponentOfBaseType(@sound))
                sound.StartOneShot();

            // delete yourself
            ezMsgDeleteGameObject del;
            GetOwner().PostMessage(del, ezTime::Seconds(0.1));
        }
    }
}

