import ez = require("../TypeScript/ez")

export class Rotate extends ez.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    private _lines: ez.Debug.Line[];

    FindObjects = (go: ez.GameObject): boolean => {
        let owner = this.GetOwner();
        let ownerPos = owner.GetGlobalPosition();
        let pos = go.GetGlobalPosition();
        //ez.Log.Info(owner.GetName() + "::FindObject: " + go.GetName() + " at " + pos.x + ", " + pos.y + ", " + pos.z + (go.HasAnyTags("game") ? " (game)" : ""));

        let line = new ez.Debug.Line();
        line.startX = ownerPos.x;
        line.startY = ownerPos.y;
        line.startZ = ownerPos.z;
        line.endX = pos.x;
        line.endY = pos.y;
        line.endZ = pos.z;

        this._lines.push(line);

        return true;
    }

    Tick(): number {

        let owner = this.GetOwner();

        this._lines = [];
        ez.World.FindObjectsInSphere("NPC", owner.GetGlobalPosition(), 5, this.FindObjects)
        ez.Debug.DrawLines(this._lines, ez.Color.OrangeRed());

        return ez.Time.Milliseconds(0);
    }
}

