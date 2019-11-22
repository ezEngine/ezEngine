import ez = require("../TypeScript/ez")

export class Trigger extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgPxTriggerTriggered, "OnMsgPxTriggerTriggered");
    }

    OnMsgPxTriggerTriggered(msg: ez.MsgPxTriggerTriggered): void {
        ez.Log.Info(this.GetOwner().GetName() +  ": " + ez.TriggerState[msg.TriggerState]);

        this.BroadcastEvent(msg);
    }    
}

