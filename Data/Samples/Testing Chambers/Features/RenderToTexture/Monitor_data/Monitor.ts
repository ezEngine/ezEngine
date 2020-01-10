import ez = require("TypeScript/ez")

export class MsgSwitchMonitor extends ez.Message {

    EZ_DECLARE_MESSAGE_TYPE;

    renderTarget: string;
    screenMaterial: string;
}

export class Monitor extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        ez.TypescriptComponent.RegisterMessageHandler(MsgSwitchMonitor, "OnMsgSwitchMonitor");
    }

    OnMsgSwitchMonitor(msg: MsgSwitchMonitor): void {
        ez.Log.Info("Got MsgSwitchMonitor: " + msg.screenMaterial);

        let owner = this.GetOwner();
        let display = owner.FindChildByName("Display");
        
        let mat = new ez.MsgSetMeshMaterial();
        mat.MaterialSlot = 0;
        mat.Material = msg.screenMaterial;

        display.SendMessage(mat);

        let activator = display.TryGetComponentOfBaseType(ez.RenderTargetActivatorComponent);
        activator.RenderTarget = msg.renderTarget;
    }
}

