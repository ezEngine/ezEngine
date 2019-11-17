import ez = require("../TypeScript/ez")

export class Player2 extends ez.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    Tick(): number {

        return ez.Time.Milliseconds(0);
    }

    static RegisterMessageHandlers() {

        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgInputActionTriggered, "OnMsgInputActionTriggered");
    }

    OnMsgInputActionTriggered(msg: ez.MsgInputActionTriggered): void {
        ez.Log.Info("Player Input: " + msg.InputActionHash);
    }
}

