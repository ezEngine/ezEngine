import ez = require("../../TypeScript/ez")
import msgs = require("Scripting/Messages")

export class Pickup extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    AddHealth: number = 25;
    AddAmmo: number = 0;
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgPxTriggerTriggered, "OnMsgPxTriggerTriggered");
    }

    OnMsgPxTriggerTriggered(msg: ez.MsgPxTriggerTriggered): void {

        if (msg.TriggerState == ez.TriggerState.Activated && msg.MsgStringHash == ez.Utils.StringToHash("Pickup")) {

            // TODO: need GO handles in messages to identify who entered the trigger
            let player = ez.World.TryGetObjectWithGlobalKey("Player");
            if (player == null)
                return;

            let hm = new msgs.MsgAddHealth();
            hm.addHealth = this.AddHealth;

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

