import ez = require("TypeScript/ez")

export class ShootingStar2 extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    ragdollFinished:boolean = false;

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        // you can only call "RegisterMessageHandler" from within this function
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgDamage, "OnMsgDamage");
    }

    OnMsgDamage(msg: ez.MsgDamage): void {

        if (!this.ragdollFinished) {
            
            var col = this.GetOwner().TryGetComponentOfBaseType(ez.JoltHitboxComponent);
            
            if (col != null) {
                // if present, deactivate the bone collider component, it isn't needed anymore
                col.SetActiveFlag(false);
            }
            
            var da = this.GetOwner().TryGetComponentOfBaseType(ez.JoltDynamicActorComponent);
            
            if (da != null) {
                // if present, deactivate the dynamic actor component, it isn't needed anymore
                da.SetActiveFlag(false);
            }            
            
            var rdc = this.GetOwner().TryGetComponentOfBaseType(ez.JoltRagdollComponent);
            
            if (rdc != null) {
                
                if (rdc.IsActiveAndSimulating()) {
                    this.ragdollFinished = true;
                    return;
                }

                rdc.StartMode = ez.JoltRagdollStartMode.WithCurrentMeshPose;
                rdc.SetActiveFlag(true);

                // we want the ragdoll to get a kick, so send an impulse message
                var imp = new ez.MsgPhysicsAddImpulse();
                imp.Impulse = msg.ImpactDirection.Clone();
                imp.Impulse.MulNumber(Math.min(msg.Damage, 5) * 10);
                imp.GlobalPosition = msg.GlobalPosition.Clone();
                rdc.SendMessage(imp);
            }
        }
    }
}

