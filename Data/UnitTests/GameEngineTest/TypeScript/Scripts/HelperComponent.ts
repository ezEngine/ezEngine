import ez = require("TypeScript/ez")
import EZ_TEST = require("./TestFramework")
import shared = require("./Shared")

export class HelperComponent extends ez.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgGenericEvent, "OnMsgGenericEvent");
    }

    // Initialize(): void { }
    // Deinitialize(): void { }
    // OnActivated(): void { }
    // OnDeactivated(): void { }

    OnSimulationStarted(): void {
        this.SetTickInterval(ez.Time.Milliseconds(0));
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "Event1") {

            let e = new ez.MsgGenericEvent;
            e.Message = "e1";

            this.BroadcastEvent(e);
        }

        // should not reach itself
        EZ_TEST.BOOL(msg.Message != "e1");
    }

    Tick(): void {
    }
}

