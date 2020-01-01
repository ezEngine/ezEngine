import ez = require("TypeScript/ez")

export class Corridor extends ez.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgGenericEvent, "OnMsgGenericEvent");
    }

    OnSimulationStarted(): void {
        this.SetTickInterval(ez.Time.Milliseconds(1000));
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "SecretDoorButton") {

            let door = ez.World.TryGetObjectWithGlobalKey("SecretDoor");
            if (door != null) {

                let slider = door.TryGetComponentOfBaseType(ez.SliderComponent);

                if (slider != null && !slider.Running) {

                    // slider direction toggles automatically, just need to set the running state again
                    slider.Running = true;
                }
            }
        }

    }

    Tick(): void {
    }
}

