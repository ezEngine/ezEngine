import ez = require("TypeScript/ez")

export class Turret extends ez.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    Health: number = 50;
    /* END AUTO-GENERATED: VARIABLES */

    target: ez.GameObject = null;
    gunSpawn: ez.SpawnComponent = null;
    gunSound: ez.FmodEventComponent = null;

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgDamage, "OnMsgDamage");
    }

    OnMsgDamage(msg: ez.MsgDamage) {

        if (this.Health <= 0)
            return;

        this.Health -= msg.Damage;

        if (this.Health > 0)
            return;

        let expObj = this.GetOwner().FindChildByName("Explosion", true);
        if (expObj == null)
            return;

        let expComp = expObj.TryGetComponentOfBaseType(ez.SpawnComponent);
        if (expComp == null)
            return;

        expComp.TriggerManualSpawn(true, ez.Vec3.ZeroVector());
    }

    OnSimulationStarted(): void {
        this.SetTickInterval(ez.Time.Milliseconds(50));

        let gun = this.GetOwner().FindChildByName("Gun", true);

        this.gunSpawn = gun.TryGetComponentOfBaseType(ez.SpawnComponent);
        this.gunSound = gun.TryGetComponentOfBaseType(ez.FmodEventComponent);
    }


    FoundObjectCallback = (go: ez.GameObject): boolean => {

        this.target = go;

        return false;
    }

    Tick(): void {

        if (this.Health <= 0)
            return;

        if (this.gunSpawn == null || !this.gunSpawn.IsValid())
            return;

        let owner = this.GetOwner();

        this.target = null;
        ez.World.FindObjectsInSphere("Player", owner.GetGlobalPosition(), 15, this.FoundObjectCallback);

        if (this.target == null)
            return;

        let dirToTarget = new ez.Vec3();
        dirToTarget.SetSub(this.target.GetGlobalPosition(), owner.GetGlobalPosition());

        let distance = dirToTarget.GetLength();

        let vis = ez.Physics.Raycast(owner.GetGlobalPosition(), dirToTarget, distance, 7, ez.Physics.ShapeType.Static);
        if (vis != null)
            return;

        let targetRotation = new ez.Quat();
        targetRotation.SetShortestRotation(ez.Vec3.UnitAxisX(), dirToTarget);

        let newRotation = new ez.Quat();
        newRotation.SetSlerp(owner.GetGlobalRotation(), targetRotation, 0.1);

        owner.SetGlobalRotation(newRotation);

        dirToTarget.Normalize();


        if (dirToTarget.Dot(owner.GetGlobalDirForwards()) > Math.cos(ez.Angle.DegreeToRadian(15))) {

            this.gunSpawn.ScheduleSpawn();
            this.gunSound.StartOneShot();
        }
    }
}

