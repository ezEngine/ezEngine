import ez = require("../TypeScript/ez")

export class Rotate extends ez.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    Speed: number = 90;
    /* END AUTO-GENERATED: VARIABLES */

    private _degree = 0;

    constructor() {
        super()
    }

    private _lines: ez.Debug.Line[];

    FindObjects = (go: ez.GameObject): boolean => {
        let owner = this.GetOwner();
        let ownerPos = owner.GetGlobalPosition();
        let pos = go.GetGlobalPosition();
        ez.Log.Info(owner.GetName() + "::FindObject: " + go.GetName() + " at " + pos.x + ", " + pos.y + ", " + pos.z + (go.HasAnyTags("game") ? " (game)" : ""));

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
        let diff = ez.Clock.GetTimeDiff();

        this._degree += diff * this.Speed;

        let mRot = new ez.Mat3();
        mRot.SetRotationMatrixZ(ez.Angle.DegreeToRadian(this._degree));

        let qRot = new ez.Quat();
        qRot.SetFromMat3(mRot);

        let owner = this.GetOwner();
        owner.SetLocalRotation(qRot);

        this._lines = [];
        ez.World.FindObjectsInSphere(owner.GetGlobalPosition(), 5, this.FindObjects)
        //ez.World.Debug.DrawLineSphere(owner.GetGlobalPosition(), 5, ez.Color.AliceBlue());
        ez.Debug.DrawLines(this._lines, ez.Color.OrangeRed());

        return ez.Time.Milliseconds(0);
    }
}

