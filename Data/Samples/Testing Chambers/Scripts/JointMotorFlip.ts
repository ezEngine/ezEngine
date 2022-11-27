import ez = require("TypeScript/ez")

export class JointMotorFlip extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    Seconds: number = 10;
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        // you can only call "RegisterMessageHandler" from within this function
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgComponentInternalTrigger, "OnMsgComponentInternalTrigger");
    }

    OnSimulationStarted(): void {

        let msg = new ez.MsgComponentInternalTrigger();
        msg.Message = "FlipMotor";

        this.PostMessage(msg, this.Seconds);

    }

    OnMsgComponentInternalTrigger(msg: ez.MsgComponentInternalTrigger): void {

        let joint = this.GetOwner().TryGetComponentOfBaseType(ez.JoltHingeConstraintComponent);

        if (joint != null) {

            joint.DriveTargetValue = -joint.DriveTargetValue;
        }

        this.PostMessage(msg, this.Seconds);
    }
}

