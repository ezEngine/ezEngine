import ez = require("TypeScript/ez")

declare function ezTestFailure(file: string, line: number, func: string, msg: string): void;

function TestFailed(msg: string) {
    let stack = (new Error).stack;
    let str = stack.split("\n")[3];
    let file = str.split(":")[0].split("(")[1];
    file = file.replace(".ts", ".js");
    let line = parseInt(str.split(":")[1]);
    let func = "";

    ezTestFailure(file, line, func, msg);
}

function EZ_TEST_BOOL(condition: boolean) {

    if (!condition) {
        TestFailed("");
    }
}

function EZ_TEST_FLOAT(f1: number, f2: number, epsilon: number) {

    if (!ez.Utils.IsNumberEqual(f1, f2, epsilon)) {
        TestFailed(f1 + " does not equal " + f2);
    }
}

function EZ_TEST_VEC2(v1: ez.Vec2, v2: ez.Vec2, epsilon: number) {

    if (!v1.IsEqual(v2, epsilon)) {
        TestFailed("(" + v1.x + ", " + v1.y + ") does not equal (" + v2.x + ", " + v2.y + ")");
    }
}

function EZ_TEST_VEC3(v1: ez.Vec3, v2: ez.Vec3, epsilon: number) {

    if (!v1.IsEqual(v2, epsilon)) {
        TestFailed("(" + v1.x + ", " + v1.y + ", " + v1.z + ") does not equal (" + v2.x + ", " + v2.y + ", " + v2.z + ")");
    }
}

export class TestBasics extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgGenericEvent, "OnMsgGenericEvent");
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "TestVec3") {

            let d = new ez.Vec3();
            EZ_TEST_FLOAT(d.x, 0, 0.001);
            EZ_TEST_FLOAT(d.y, 0, 0.001);
            EZ_TEST_FLOAT(d.z, 0, 0.001);

            EZ_TEST_VEC3(new ez.Vec3(), ez.Vec3.ZeroVector(), 0.0001);

            let v = new ez.Vec3(1, 2, 3);
            EZ_TEST_FLOAT(v.x, 1, 0.001);
            EZ_TEST_FLOAT(v.y, 2, 0.001);
            EZ_TEST_FLOAT(v.z, 3, 0.001);

            EZ_TEST_VEC3(v.Clone(), v, 0.001);

            EZ_TEST_VEC2(v.CloneAsVec2(), new ez.Vec2(1, 2), 0.001);

            EZ_TEST_VEC3(new ez.Vec3(1, 1, 1), ez.Vec3.OneVector(), 0.0001);

            EZ_TEST_VEC3(new ez.Vec3(1, 0, 0), ez.Vec3.UnitAxisX(), 0.0001);
            EZ_TEST_VEC3(new ez.Vec3(0, 1, 0), ez.Vec3.UnitAxisY(), 0.0001);
            EZ_TEST_VEC3(new ez.Vec3(0, 0, 1), ez.Vec3.UnitAxisZ(), 0.0001);

            v.Set(4, 5, 6);
            EZ_TEST_FLOAT(v.x, 4, 0.001);
            EZ_TEST_FLOAT(v.y, 5, 0.001);
            EZ_TEST_FLOAT(v.z, 6, 0.001);

            let v2 = new ez.Vec3();
            v2.SetVec3(v);
            EZ_TEST_VEC3(v, v2, 0.0001);

            v2.SetAll(7);
            EZ_TEST_FLOAT(v2.x, 7, 0.001);
            EZ_TEST_FLOAT(v2.y, 7, 0.001);
            EZ_TEST_FLOAT(v2.z, 7, 0.001);

            v2.SetZero();
            EZ_TEST_FLOAT(v2.x, 0, 0.001);
            EZ_TEST_FLOAT(v2.y, 0, 0.001);
            EZ_TEST_FLOAT(v2.z, 0, 0.001);

            EZ_TEST_FLOAT(v2.GetLengthSquared(), 0, 0.001);
            v2.SetAll(1);

            EZ_TEST_FLOAT(v2.GetLengthSquared(), 3, 0.001);

            EZ_TEST_FLOAT(v2.GetLength(), Math.sqrt(3), 0.001);

            let l = v2.GetLengthAndNormalize();
            EZ_TEST_FLOAT(l, Math.sqrt(3), 0.001);
            EZ_TEST_FLOAT(v2.GetLength(), 1, 0.001);

            EZ_TEST_BOOL(!v.IsNormalized());
            v.Normalize();
            EZ_TEST_FLOAT(v.GetLength(), 1, 0.001);
            EZ_TEST_BOOL(v.IsNormalized());

            v.Set(3, 0, 0);
            EZ_TEST_VEC3(v.GetNormalized(), ez.Vec3.UnitAxisX(), 0.0001);

            EZ_TEST_BOOL(v.NormalizeIfNotZero(ez.Vec3.UnitAxisZ(), 0.001));
            EZ_TEST_VEC3(v, ez.Vec3.UnitAxisX(), 0.0001);

            EZ_TEST_BOOL(!v.IsZero());
            v.SetZero();
            EZ_TEST_BOOL(v.IsZero());

            EZ_TEST_BOOL(!v.NormalizeIfNotZero(ez.Vec3.UnitAxisZ(), 0.001));
            EZ_TEST_VEC3(v, ez.Vec3.UnitAxisZ(), 0.0001);
            
            v.Set(1, 2, 3);
            EZ_TEST_VEC3(v.GetNegated(), new ez.Vec3(-1, -2, -3), 0.0001);
            EZ_TEST_VEC3(v, new ez.Vec3(1, 2, 3), 0.0001);

            v.Negate();
            EZ_TEST_VEC3(v, new ez.Vec3(-1, -2, -3), 0.0001);


            msg.Message = "done";
        }
    }

}

