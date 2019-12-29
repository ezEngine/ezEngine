import ez = require("<PATH-TO-EZ-TS>")

//export class NewComponent extends ez.TypescriptComponent {
export class NewComponent extends ez.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        // you can only call "RegisterMessageHandler" from within this function
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgSetColor, "OnMsgSetColor");
    }

    // Initialize(): void { }
    // Deinitialize(): void { }
    // OnActivated(): void { }
    // OnDeactivated(): void { }

    OnSimulationStarted(): void {
        this.SetTickInterval(ez.Time.Milliseconds(100));
    }

    OnMsgSetColor(msg: ez.MsgSetColor): void {
        ez.Log.Info("MsgSetColor: " + msg.Color.r + ", " + msg.Color.g + ", " + msg.Color.b + ", " + msg.Color.a);
    }

    Tick(): void {
        // if a regular tick is not needed, remove this and derive directly from ez.TypescriptComponent
        ez.Log.Info("NewComponent.Tick()")
    }
}

