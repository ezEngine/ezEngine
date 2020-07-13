import ez = require("TypeScript/ez")

export class JointBroke extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    OnBreakMsg: string = "Joint Broke !";
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgPhysicsJointBroke, "OnMsgPhysicsJointBroke");
    }

    OnMsgPhysicsJointBroke(msg: ez.MsgPhysicsJointBroke): void {
        ez.Log.Info(this.OnBreakMsg);
    }
}

