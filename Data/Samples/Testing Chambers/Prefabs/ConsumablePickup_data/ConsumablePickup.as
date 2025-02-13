#include "../../Scripting/GameDecls.as"

class ConsumablePickup : ezAngelScriptClass
{
    int ObjectType = 0;
    int Amount = 0;

    void OnMsgTriggerTriggered(ezMsgTriggerTriggered@ msg)
    {
        if (msg.TriggerState == ezTriggerState::Activated && msg.Message == "Pickup")
        {
            // TODO: need GO handles in messages to identify who entered the trigger
            ezGameObject@ player;
            if (!GetWorld().TryGetObjectWithGlobalKey("Player", player))
                return;

            MsgAddConsumable hm;
            hm.consumableType = ConsumableType(ObjectType);
            hm.amount = Amount;

            player.SendMessageRecursive(hm);

            if (hm.return_consumed == false)
                return;

            ezFmodEventComponent@ sound;
            if (GetOwner().TryGetComponentOfBaseType(@sound))
            {
                sound.StartOneShot();
            }

            ezMsgDeleteGameObject del;
            GetOwner().PostMessage(del, ezTime::Seconds(0.1));
        }
    }
}

