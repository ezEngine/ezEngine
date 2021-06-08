import ez = require("../../TypeScript/ez")

export class HealthPickup extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    pfxTop: ez.ParticleComponent = null;
    pfxPickup: ez.ParticleComponent = null;

    OnSimulationStarted(): void {
        this.pfxTop = this.GetOwner().FindChildByName("Particle", true).TryGetComponentOfBaseType(ez.ParticleComponent);
        this.pfxPickup = this.GetOwner().TryGetComponentOfBaseType(ez.ParticleComponent);
    }

    static RegisterMessageHandlers() {

        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgGenericEvent, "OnMsgGenericEvent");
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgTriggerTriggered, "OnMsgTriggerTriggered");
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "Animation Cue 1") {
            this.pfxTop.StartEffect();
        }
    }    

    OnMsgTriggerTriggered(msg: ez.MsgTriggerTriggered): void {

        if (msg.TriggerState == ez.TriggerState.Activated && msg.Message == "Pickup") {

            this.pfxPickup.StartEffect();

            let del = new ez.MsgDeleteGameObject();
            this.GetOwner().PostMessage(del, 0.1);
        }
    }    
}

