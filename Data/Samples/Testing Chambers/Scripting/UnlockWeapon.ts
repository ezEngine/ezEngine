import ez = require("TypeScript/ez")
import _gm = require("Scripting/GameMessages")

export class UnlockWeapon extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    WeaponType: number = 0;
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgTriggerTriggered, "OnMsgTriggerTriggered");
    }

    OnMsgTriggerTriggered(msg: ez.MsgTriggerTriggered): void {

        if (msg.TriggerState == ez.TriggerState.Activated && msg.MsgStringHash == ez.Utils.StringToHash("Pickup")) {

            // TODO: need GO handles in messages to identify who entered the trigger
            let player = ez.World.TryGetObjectWithGlobalKey("Player");
            if (player == null)
                return;

            let hm = new _gm.MsgUnlockWeapon();
            hm.WeaponType = this.WeaponType;

            player.SendMessage(hm, true);

            if (hm.return_consumed == false)
                return;

            let sound = this.GetOwner().TryGetComponentOfBaseType(ez.FmodEventComponent);
            if (sound != null)
                sound.StartOneShot();

            let del = new ez.MsgDeleteGameObject();
            this.GetOwner().PostMessage(del, 0.1);
        }
    }
}

