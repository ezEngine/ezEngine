import ez = require("<PATH-TO-EZ-TS>")

export class NewComponent extends ez.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()

        
    }

    Tick(): void {
        ez.Log.Info("NewComponent.Tick()")
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
        this.SetTickInterval(ez.Time.Milliseconds(100));
    }

    // to use message handlers you must implement exactly this function
    static RegisterMessageHandlers() {
        // you can only call "RegisterMessageHandler" from within this function
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgSetColor, "OnMsgSetColor");
    }

    // example message handler
    OnMsgSetColor(msg: ez.MsgSetColor): void {
        ez.Log.Info("MsgSetColor: " + msg.Color.r + ", " + msg.Color.g + ", " + msg.Color.b + ", " + msg.Color.a);
    }    
    */
}

