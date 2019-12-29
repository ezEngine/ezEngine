import ez = require("TypeScript/ez")

declare function ezTestFailure(): void;

function EZ_TEST_BOOL(condition: boolean) {

    if (!condition) {
        ezTestFailure();
    }
}

export class TestBasics extends ez.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    Tick(): void {
        ez.Log.Info("TestBasics.Tick()")
    }

    /*
    Initialize(): void {
    }

    Deinitialize(): void {
    }

    OnActivated(): void {
    }

    OnDeactivated(): void {
    }

    OnSimulationStarted(): void {
    }
*/
    static RegisterMessageHandlers() {
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgGenericEvent, "OnMsgGenericEvent");
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {
        if (msg.Message == "TestVec3") {
            ez.Log.Info("Test Vec3");

            EZ_TEST_BOOL(false);

            msg.Message = "done";
        }
    }

}

