import ez = require("../../TypeScript/ez")

const enum BallMineState {
    Idle,
    Alert,
    Approaching,
    Attacking
};

export class BallMine extends ez.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    AlertDistance: number = 15;
    ApproachDistance: number = 10;
    AttackDistance: number = 1.5;
    RollForce: number = 100;
    Health: number = 20;
    /* END AUTO-GENERATED: VARIABLES */

    private _player: ez.GameObject;
    private _state = BallMineState.Idle;

    constructor() {
        super()
    }

    OnSimulationStarted(): void {
        this._player = ez.World.TryGetObjectWithGlobalKey("Player");
    }

    Tick(): number {

        let oldState = this._state;

        if (this._player != null && this._player.IsValid()) {

            let playerPos = this._player.GetGlobalPosition();
            let ownPos = this.GetOwner().GetGlobalPosition();
            let diffPos = new ez.Vec3();

            diffPos.SetSub(playerPos, ownPos);

            let distToPlayer = diffPos.GetLength();

            //ez.Log.Dev("Distance to Player: " + distToPlayer);

            if (distToPlayer <= this.ApproachDistance) {

                this._state = BallMineState.Approaching;

                let actor = this.GetOwner().TryGetComponentOfBaseType(ez.PxDynamicActorComponent);
                if (actor != null) {
                    diffPos.Normalize();
                    diffPos.MulNumber(this.RollForce);

                    actor.AddLinearForce(diffPos);
                }

                //ez.Log.Dev("Attack: " + distToPlayer);
            }
            else if (distToPlayer <= this.AlertDistance) {
                this._state = BallMineState.Alert;
                //ez.Log.Dev("Alert: " + distToPlayer);
            }
            else {
                this._state = BallMineState.Idle;

            }

            if (distToPlayer <= this.AttackDistance) {
                this._state = BallMineState.Attacking;
            }
        }
        else {
            this._state = BallMineState.Idle;
        }

        if (oldState != this._state) {

            switch (this._state) {
                case BallMineState.Idle:
                    {
                        let matMsg = new ez.MsgSetMeshMaterial();
                        matMsg.Material = "{ d615cd66-0904-00ca-81f9-768ff4fc24ee }";
                        this.GetOwner().SendMessageRecursive(matMsg);

                        return ez.Time.Milliseconds(500);
                    }
                case BallMineState.Alert:
                    {
                        let matMsg = new ez.MsgSetMeshMaterial();
                        matMsg.Material = "{ 6ae73fcf-e09c-1c3f-54a8-8a80498519fb }";
                        this.GetOwner().SendMessageRecursive(matMsg);

                        return ez.Time.Milliseconds(100);
                    }
                case BallMineState.Approaching:
                    {
                        let matMsg = new ez.MsgSetMeshMaterial();
                        matMsg.Material = "{ 49324140-a093-4a75-9c6c-efde65a39fc4 }";
                        this.GetOwner().SendMessageRecursive(matMsg);

                        return ez.Time.Milliseconds(50);
                    }
                case BallMineState.Attacking:
                    {
                        this.Explode();

                        return ez.Time.Milliseconds(50);
                    }

            }
        }
    }

    Explode(): void {
        let spawnExpl = this.GetOwner().TryGetComponentOfBaseType(ez.SpawnComponent);

        if (spawnExpl != null) {
            spawnExpl.TriggerManualSpawn();
        }

        ez.World.DeleteObjectDelayed(this.GetOwner());
    }

    // to use message handlers you must implement exactly this function
    static RegisterMessageHandlers() {
        // you can only call "RegisterMessageHandler" from within this function
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgDamage, "OnMsgDamage");
    }

    // example message handler
    OnMsgDamage(msg: ez.MsgDamage): void {
        if (this.Health > 0) {
            this.Health -= msg.Damage;

            if (this.Health <= 0) {
                this.Explode();
            }
        }
    }
}

