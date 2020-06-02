import ez = require("TypeScript/ez")

export class Turret extends ez.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    Range: number = 3;
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        // you can only call "RegisterMessageHandler" from within this function
        //ez.TypescriptComponent.RegisterMessageHandler(ez.MsgSetColor, "OnMsgSetColor");
    }

    OnSimulationStarted(): void {

        // update this component every frame
        this.SetTickInterval(ez.Time.Zero());
    }

    // OnMsgSetColor(msg: ez.MsgSetColor): void {
    //     ez.Log.Info("MsgSetColor: " + msg.Color.r + ", " + msg.Color.g + ", " + msg.Color.b + ", " + msg.Color.a);
    // }

    allTargets: ez.GameObject[] = [];

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
}

