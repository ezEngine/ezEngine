import ez = require("TypeScript/ez")

export class GasCylinder extends ez.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    static RegisterMessageHandlers() {

        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgDamage, "OnMsgDamage");
    }

    private capHealth = 5;
    private bodyHealth = 50;

    constructor() {
        super()
    }

    OnSimulationStarted(): void {
        this.SetTickInterval(ez.Time.Milliseconds(100));
    }

    Tick(): void {

        if (this.capHealth <= 0) {

            let owner = this.GetOwner();
            let cap = owner.FindChildByName("Cap");

            let forceMsg = new ez.MsgPhysicsAddForce();

            forceMsg.GlobalPosition = cap.GetGlobalPosition();
            forceMsg.Force = cap.GetGlobalDirUp();

            let randomDir = ez.Vec3.CreateRandomDirection();
            randomDir.MulNumber(0.6);

            forceMsg.Force.AddVec3(randomDir);
            forceMsg.Force.MulNumber(-400);

            owner.SendMessage(forceMsg);
        }
    }

    OnMsgDamage(msg: ez.MsgDamage) {

        //ez.Log.Info("Damaged: " + msg.HitObjectName + " - " + msg.Damage)

        this.bodyHealth -= msg.Damage;

        if (this.bodyHealth <= 0) {
            this.Explode();
            return;
        }

        if (msg.HitObjectName == "Cap") {

            if (this.capHealth > 0) {

                this.capHealth -= msg.Damage;

                if (this.capHealth <= 0) {

                    this.SetTickInterval(ez.Time.Milliseconds(0));

                    let leakObj = this.GetOwner().FindChildByName("LeakEffect");
                    if (leakObj != null) {

                        let leakFX = leakObj.TryGetComponentOfBaseType(ez.ParticleComponent);

                        if (leakFX != null) {
                            leakFX.StartEffect();
                        }
                    }

                    // trigger code path below
                    msg.HitObjectName = "Tick";
                }
            }
        }

        if (msg.HitObjectName == "Tick") {

            let tickDmg = new ez.MsgDamage();
            tickDmg.Damage = 1;
            tickDmg.HitObjectName = "Tick";
            this.PostMessage(tickDmg, ez.Time.Milliseconds(100));

        }
    }

    Explode(): void {

        let owner = this.GetOwner();
        let exp = owner.FindChildByName("Explosion");

        if (exp != null) {

            let spawnExpl = exp.TryGetComponentOfBaseType(ez.SpawnComponent);

            if (spawnExpl != null) {
                spawnExpl.TriggerManualSpawn(false, ez.Vec3.ZeroVector());
            }
        }

        ez.World.DeleteObjectDelayed(this.GetOwner());
    }
}

