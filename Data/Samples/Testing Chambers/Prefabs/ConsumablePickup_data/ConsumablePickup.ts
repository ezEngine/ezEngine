import ez = require("TypeScript/ez")
import _gm = require("Scripting/GameMessages")

export class ConsumablePickup extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    ConsumableType: number = 0;
    Amount: number = 0;
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgTriggerTriggered, "OnMsgTriggerTriggered");
    }

    OnMsgTriggerTriggered(msg: ez.MsgTriggerTriggered): void {

        if (msg.TriggerState == ez.TriggerState.Activated && msg.Message == "Pickup") {

            // TODO: need GO handles in messages to identify who entered the trigger
            let player = ez.World.TryGetObjectWithGlobalKey("Player");
            if (player == null)
                return;

            let hm = new _gm.MsgAddConsumable();
            hm.consumableType = this.ConsumableType;
            hm.amount = this.Amount;

            player.SendMessage(hm, true);

            if (hm.return_consumed == false)
                return;

            let sound = this.GetOwner().TryGetComponentOfBaseType(ez.FmodEventComponent);
            sound.StartOneShot();

            let del = new ez.MsgDeleteGameObject();
            this.GetOwner().PostMessage(del, 0.1);
        }
    }
}

