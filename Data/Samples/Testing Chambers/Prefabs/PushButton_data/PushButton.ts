import ez = require("TypeScript/ez")

export class PushButton extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    ButtonName: string = "";
    /* END AUTO-GENERATED: VARIABLES */

    slider: ez.TransformComponent = null;

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgGenericEvent, "OnMsgGenericEvent");
    }

    OnSimulationStarted(): void {

        let owner = this.GetOwner();
        let button = owner.FindChildByName("Button");
        this.slider = button.TryGetComponentOfBaseType(ez.TransformComponent);
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "Use") {

            if (this.slider == null || this.slider.Running)
                return;

            this.slider.SetDirectionForwards(true);
            this.slider.Running = true;

            let newMsg = new ez.MsgGenericEvent();
            newMsg.Message = this.ButtonName;

            this.BroadcastEvent(newMsg);
        }
    }
}

