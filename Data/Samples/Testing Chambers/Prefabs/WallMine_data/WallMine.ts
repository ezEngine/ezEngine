import ez = require("../../TypeScript/ez")

export class WallMine extends ez.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    private distance = 0;

    constructor() {
        super()
    }

    OnSimulationStarted(): void {
        this.SetTickInterval(ez.Time.Milliseconds(40));
    }

    Tick(): void {

        let owner = this.GetOwner();
        let pos = owner.GetGlobalPosition();
        let dir = owner.GetGlobalDirForwards();

        let shapeId = -1;
        let staticactor = owner.TryGetComponentOfBaseType(ez.JoltStaticActorComponent);
        if (staticactor != null) {
            shapeId = staticactor.GetObjectFilterID();
        }

        let res = ez.Physics.Raycast(pos, dir, 10, 0, ez.Physics.ShapeType.Static | ez.Physics.ShapeType.AllInteractive , shapeId);

        if (res == null) {
            return;
        }

        if (res.distance < this.distance - 0.05) {
            // allow some slack

            this.Explode();
        }
        else if (res.distance > this.distance) {

            let glowLine = owner.FindChildByName("GlowLine", false);

            if (glowLine != null) {
                glowLine.SetLocalScaling(new ez.Vec3(res.distance, 1, 1));
                glowLine.SetLocalPosition(new ez.Vec3(res.distance * 0.5, 0, 0));
            }

            this.distance = res.distance;
        }
    }

    Explode(): void {

        let owner = this.GetOwner();
        let exp = owner.FindChildByName("Explosion");

        if (exp != null) {

            let spawnExpl = exp.TryGetComponentOfBaseType(ez.SpawnComponent);

            if (spawnExpl != null) {
                spawnExpl.TriggerManualSpawn(true, ez.Vec3.ZeroVector());
            }
        }

        ez.World.DeleteObjectDelayed(this.GetOwner());
    }

    // to use message handlers you must implement exactly this function
    static RegisterMessageHandlers() {
        // you can only call "RegisterMessageHandler" from within this function
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgDamage, "OnMsgDamage");
    }

    OnMsgDamage(msg: ez.MsgDamage): void {
        // explode on any damage
        this.Explode();
    }
}

