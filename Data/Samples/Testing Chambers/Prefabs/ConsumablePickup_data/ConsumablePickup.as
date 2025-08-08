#include "../../Scripts/GameDecls.as"

class ConsumablePickup : ezAngelScriptClass
{
    int ObjectType = 0;
    int Amount = 0;

    void OnMsgTriggerTriggered(ezMsgTriggerTriggered@ msg)
    {
        if (msg.TriggerState == ezTriggerState::Activated && msg.Message == "Pickup")
        {
            MsgAddConsumable hm;
            hm.consumableType = ConsumableType(ObjectType);
            hm.amount = Amount;

            GetWorld().SendMessageRecursive(msg.GameObject, hm);

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

