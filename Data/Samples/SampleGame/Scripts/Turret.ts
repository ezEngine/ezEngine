import ez = require("TypeScript/ez")

export class Turret extends ez.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    Range: number = 3;
    /* END AUTO-GENERATED: VARIABLES */

    allTargets: ez.GameObject[] = [];
    lastDamageTime: number;

    constructor() {
        super()
    }

    OnSimulationStarted(): void {

        // update this component every frame
        this.SetTickInterval(ez.Time.Zero());
        this.lastDamageTime = ez.Time.GetGameTime();
    }

    FoundTargetCallback = (go: ez.GameObject): boolean => {
        this.allTargets.push(go);
        return true;
    }

    Tick(): void {

        let owner = this.GetOwner();

        // find all objects with the 'TurretTarget' marker that are close by
        this.allTargets = [];
        ez.World.FindObjectsInSphere("TurretTarget", owner.GetGlobalPosition(), this.Range, this.FoundTargetCallback);

        this.DrawLinesToTargets();

        if (ez.Time.GetGameTime() - this.lastDamageTime > ez.Time.Milliseconds(40)) {
            
            this.lastDamageTime = ez.Time.GetGameTime();

            this.DamageAllTargets(4);
        }
    }

    DrawLinesToTargets(): void {

        const startPos = this.GetOwner().GetGlobalPosition();

        let lines: ez.Debug.Line[] = [];

        for (let i = 0; i < this.allTargets.length; ++i) {

            const target = this.allTargets[i];
            const endPos = target.GetGlobalPosition();

            let line = new ez.Debug.Line();
            line.startX = startPos.x;
            line.startY = startPos.y;
            line.startZ = startPos.z;
            line.endX = endPos.x;
            line.endY = endPos.y;
            line.endZ = endPos.z;

            lines.push(line);
        }

        ez.Debug.DrawLines(lines, ez.Color.OrangeRed());
    }

    DamageAllTargets(damage: number): void {

        let dmgMsg = new ez.MsgDamage();
        dmgMsg.Damage = damage;

        for (let i = 0; i < this.allTargets.length; ++i) {

            this.allTargets[i].SendMessage(dmgMsg);
        }
    }
}

