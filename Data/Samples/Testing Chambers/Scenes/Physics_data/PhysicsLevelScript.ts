import ez = require("TypeScript/ez")

export class PhysicsLevelScript extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        // you can only call "RegisterMessageHandler" from within this function
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgTriggerTriggered, "OnMsgTriggerTriggered");
    }

    OnSimulationStarted(): void {
    }

    OnMsgTriggerTriggered(msg: ez.MsgTriggerTriggered): void {

        if (msg.Message == "ActivatePaddleWheel") {

            if (msg.TriggerState == ez.TriggerState.Activated) {

                let spawn1 = ez.World.TryGetObjectWithGlobalKey("PaddleWheelSpawn1");
                if (spawn1 != null) {
                    spawn1.SetActiveFlag(true);
                }

            }
            else if (msg.TriggerState == ez.TriggerState.Deactivated) {

                let spawn1 = ez.World.TryGetObjectWithGlobalKey("PaddleWheelSpawn1");
                if (spawn1 != null) {
                    spawn1.SetActiveFlag(false);
                }

            }
        }

        if (msg.Message == "ActivateSwing") {

            if (msg.TriggerState == ez.TriggerState.Activated) {

                let spawn1 = ez.World.TryGetObjectWithGlobalKey("SwingSpawn1");
                if (spawn1 != null) {
                    spawn1.SetActiveFlag(true);
                }

            }
            else if (msg.TriggerState == ez.TriggerState.Deactivated) {

                let spawn1 = ez.World.TryGetObjectWithGlobalKey("SwingSpawn1");
                if (spawn1 != null) {
                    spawn1.SetActiveFlag(false);
                }

            }
        }
    }
}

