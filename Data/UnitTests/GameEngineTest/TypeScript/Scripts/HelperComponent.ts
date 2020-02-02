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

    RaiseEvent(text: string): void {
        let e = new ez.MsgGenericEvent;
        e.Message = text;
        this.BroadcastEvent(e);
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "Event1") {

            this.RaiseEvent("e1");
        }

        // should not reach itself
        EZ_TEST.BOOL(msg.Message != "e1");
    }

    Tick(): void {
    }
}

