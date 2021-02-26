import ez = require("TypeScript/ez")

export class TargetSphere extends ez.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        // you can only call "RegisterMessageHandler" from within this function
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgDamage, "OnMsgDamage");
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgInputActionTriggered, "OnMsgInputActionTriggered");
    }

    curDamage = 0;
    fireFX: ez.ParticleComponent = null;

    OnSimulationStarted(): void {
        this.SetTickInterval(ez.Time.Milliseconds(100));

        this.fireFX = this.GetOwner().TryGetComponentOfBaseType(ez.ParticleComponent);
    }

    OnMsgDamage(msg: ez.MsgDamage): void {

        this.curDamage += msg.Damage;
    }

    OnMsgInputActionTriggered(msg: ez.MsgInputActionTriggered) {
     
        if (msg.TriggerState == ez.TriggerState.Activated) {
            if (msg.InputAction == "Heal") {
                this.curDamage = 0;
            }
        }
    }

    Tick(): void {

        this.curDamage = ez.Utils.Clamp(this.curDamage - 1.0, 0, 1000);
        const dmg = this.curDamage / 100.0;

        let msgCol = new ez.MsgSetColor();
        msgCol.Color.SetLinearRGBA(dmg, dmg * 0.05, dmg * 0.05);

        this.GetOwner().SendMessageRecursive(msgCol);

        if (this.fireFX != null && this.fireFX.IsValid()) {

            if (dmg > 1.0) {
                if (!this.fireFX.IsEffectActive()) {
                    this.fireFX.StartEffect();
                }
            }
            else if (dmg < 0.8) {
                this.fireFX.StopEffect();
            }
        }
    }
}

