#include "GameDecls.as"

class UnlockWeapon : ezAngelScriptClass
{
    int weaponType = 0;

    void OnMsgTriggerTriggered(ezMsgTriggerTriggered@ msg)
    {
        if (msg.TriggerState == ezTriggerState::Activated && msg.Message == "Pickup")
        {
            // TODO: need GO handles in messages to identify who entered the trigger
            ezGameObject@ player;
            if (!GetWorld().TryGetObjectWithGlobalKey("Player", @player))
                return;

            MsgUnlockWeapon hm;
            hm.weaponType = WeaponType(weaponType);

            player.SendMessageRecursive(hm);

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

