import ez = require("TypeScript/ez")

export class ShootingStar2 extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    ragdoll:boolean = false;

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        // you can only call "RegisterMessageHandler" from within this function
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgDamage, "OnMsgDamage");
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgPhysicsAddImpulse, "OnMsgImpulse");
    }

    OnMsgImpulse(msg: ez.MsgPhysicsAddImpulse): void {

        ez.Log.Info("impulse")
        
        var rdc = this.GetOwner().TryGetComponentOfBaseType(ez.PxRagdollComponent);
        
        if (rdc != null) {
            ez.Log.Info("impulse forwarded")
            rdc.SendMessage(msg)
        }
}

    OnMsgDamage(msg: ez.MsgDamage): void {

        if (!this.ragdoll) {
            this.ragdoll = true;
            
            var rdc = this.GetOwner().TryGetComponentOfBaseType(ez.PxRagdollComponent);
            
            var imp = new ez.MsgPhysicsAddImpulse();
            imp.Impulse = msg.ImpactDirection.Clone();
            imp.Impulse.MulNumber(50);
            imp.GlobalPosition = msg.GlobalPosition.Clone();

            if (rdc != null) {
                rdc.Start = ez.PxRagdollStart.WaitForPose;
                rdc.SendMessage(imp);
            }
        }
    }
}

