import ez = require("TypeScript/ez")

export class BreakOnDamage extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    Health: number = 10;
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgDamage, "OnMsgDamage");
    }

    OnMsgDamage(msg: ez.MsgDamage): void {
        
        if (this.Health > 0) {

            this.Health -= msg.Damage;

            if (this.Health <= 0) {

                let spawnNode = this.GetOwner().FindChildByName("OnBreakSpawn");
                if (spawnNode != null) {

                    let spawnComp = spawnNode.TryGetComponentOfBaseType(ez.SpawnComponent);

                    if (spawnComp != null) {

                        let offset = ez.Vec3.CreateRandomPointInSphere();
                        offset.MulNumber(0.3);
                        spawnComp.TriggerManualSpawn(true, offset);
                    }
                }

                ez.World.DeleteObjectDelayed(this.GetOwner());
            }
        }
    }
}

