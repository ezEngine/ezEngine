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

    TestVec2(): void {

        // constructor
        let d = new ez.Vec2();
        EZ_TEST_FLOAT(d.x, 0, 0.001);
        EZ_TEST_FLOAT(d.y, 0, 0.001);

        // ZeroVector
        EZ_TEST_VEC2(new ez.Vec2(), ez.Vec2.ZeroVector(), 0.0001);

        let v = new ez.Vec2(1, 2);
        EZ_TEST_FLOAT(v.x, 1, 0.001);
        EZ_TEST_FLOAT(v.y, 2, 0.001);

        // Clone
        EZ_TEST_VEC2(v.Clone(), v, 0.001);

        // TODO: CloneAsVec3
        //EZ_TEST_VEC3(v.CloneAsVec3(3), new ez.Vec3(1, 2, 3), 0.001);

        // OneVector
        EZ_TEST_VEC2(new ez.Vec2(1, 1), ez.Vec2.OneVector(), 0.0001);

        // UnitAxisX
        EZ_TEST_VEC2(new ez.Vec2(1, 0), ez.Vec2.UnitAxisX(), 0.0001);

        // UnitAxisY
        EZ_TEST_VEC2(new ez.Vec2(0, 1), ez.Vec2.UnitAxisY(), 0.0001);

        // Set
        v.Set(4, 5);
        EZ_TEST_FLOAT(v.x, 4, 0.001);
        EZ_TEST_FLOAT(v.y, 5, 0.001);

        // SetVec2
        let v2 = new ez.Vec2();
        v2.SetVec2(v);
        EZ_TEST_VEC2(v, v2, 0.0001);

        // SetAll
        v2.SetAll(7);
        EZ_TEST_FLOAT(v2.x, 7, 0.001);
        EZ_TEST_FLOAT(v2.y, 7, 0.001);

        // SetZero
        v2.SetZero();
        EZ_TEST_FLOAT(v2.x, 0, 0.001);
        EZ_TEST_FLOAT(v2.y, 0, 0.001);

        // GetLengthSquared
        EZ_TEST_FLOAT(v2.GetLengthSquared(), 0, 0.001);
        v2.SetAll(1);

        EZ_TEST_FLOAT(v2.GetLengthSquared(), 2, 0.001);

        // GetLength
        EZ_TEST_FLOAT(v2.GetLength(), Math.sqrt(2), 0.001);

        // GetLengthAndNormalize
        let l = v2.GetLengthAndNormalize();
        EZ_TEST_FLOAT(l, Math.sqrt(2), 0.001);
        EZ_TEST_FLOAT(v2.GetLength(), 1, 0.001);

        // IsNormalized
        EZ_TEST_BOOL(!v.IsNormalized());

        // Normalize
        v.Normalize();

        EZ_TEST_FLOAT(v.GetLength(), 1, 0.001);
        EZ_TEST_BOOL(v.IsNormalized());


        // GetNormalized
        v.Set(3, 0);
        EZ_TEST_VEC2(v.GetNormalized(), ez.Vec2.UnitAxisX(), 0.0001);

        // NormalizeIfNotZero
        EZ_TEST_BOOL(v.NormalizeIfNotZero(ez.Vec2.UnitAxisY(), 0.001));
        EZ_TEST_VEC2(v, ez.Vec2.UnitAxisX(), 0.0001);

        // IsZero
        EZ_TEST_BOOL(!v.IsZero());

        // SetZero
        v.SetZero();
        EZ_TEST_BOOL(v.IsZero());

        EZ_TEST_BOOL(!v.NormalizeIfNotZero(ez.Vec2.UnitAxisY(), 0.001));
        EZ_TEST_VEC2(v, ez.Vec2.UnitAxisY(), 0.0001);

        // GetNegated
        v.Set(1, 2);
        EZ_TEST_VEC2(v.GetNegated(), new ez.Vec2(-1, -2), 0.0001);
        EZ_TEST_VEC2(v, new ez.Vec2(1, 2), 0.0001);

        // Negate
        v.Negate();
        EZ_TEST_VEC2(v, new ez.Vec2(-1, -2), 0.0001);

        // AddVec2
        v.Set(2, 3);
        v2.Set(5, 6);
        v.AddVec2(v2);
        EZ_TEST_VEC2(v, new ez.Vec2(7, 9), 0.0001);

        // SubVec2
        v.SubVec2(v2);
        EZ_TEST_VEC2(v, new ez.Vec2(2, 3), 0.0001);

        // MulVec2
        v.MulVec2(v);
        EZ_TEST_VEC2(v, new ez.Vec2(4, 9), 0.0001);

        // DivVec2
        v.DivVec2(new ez.Vec2(2, 3));
        EZ_TEST_VEC2(v, new ez.Vec2(2, 3), 0.0001);

        // MulNumber
        v.MulNumber(2);
        EZ_TEST_VEC2(v, new ez.Vec2(4, 6), 0.0001);

        // DivNumber
        v.DivNumber(2);
        EZ_TEST_VEC2(v, new ez.Vec2(2, 3), 0.0001);

        // IsIdentical
        EZ_TEST_BOOL(v.IsIdentical(v));
        EZ_TEST_BOOL(!v.IsIdentical(v2));

        // IsEqual
        EZ_TEST_BOOL(v.IsEqual(new ez.Vec2(2, 3), 0.0001));
        EZ_TEST_BOOL(!v.IsEqual(new ez.Vec2(2, 3.5), 0.0001));

        // Dot
        v.Set(2, 3);
        v2.Set(3, 4);
        EZ_TEST_FLOAT(v.Dot(v2), 18, 0.001);

        // GetCompMin
        v.Set(2, 4);
        v2.Set(1, 5);
        EZ_TEST_VEC2(v.GetCompMin(v2), new ez.Vec2(1, 4), 0.001);

        // GetCompMax
        v.Set(2, 4);
        v2.Set(1, 5);
        EZ_TEST_VEC2(v.GetCompMax(v2), new ez.Vec2(2, 5), 0.001);

        // GetCompClamp
        EZ_TEST_VEC2(v.GetCompClamp(new ez.Vec2(3, 4), new ez.Vec2(4, 5)), new ez.Vec2(3, 4), 0.001);

        // GetCompMul
        EZ_TEST_VEC2(v.GetCompMul(new ez.Vec2(2, 3)), new ez.Vec2(4, 12), 0.001);

        // GetCompDiv
        EZ_TEST_VEC2(v.GetCompDiv(new ez.Vec2(2, 4)), ez.Vec2.OneVector(), 0.001);

        // GetAbs
        v.Set(-1, -2);
        EZ_TEST_VEC2(v.GetAbs(), new ez.Vec2(1, 2), 0.001);

        // SetAbs
        v2.SetAbs(v);
        EZ_TEST_VEC2(v2, new ez.Vec2(1, 2), 0.001);

        // GetReflectedVector
        v.Set(1, 1);
        v2 = v.GetReflectedVector(new ez.Vec2(0, -1));
        EZ_TEST_VEC2(v2, new ez.Vec2(1, -1), 0.0001);

        // SetAdd
        v.SetAdd(new ez.Vec2(1, 2), new ez.Vec2(4, 5));
        EZ_TEST_VEC2(v, new ez.Vec2(5, 7), 0.0001);

        // SetSub
        v.SetSub(new ez.Vec2(4, 5), new ez.Vec2(1, 2));
        EZ_TEST_VEC2(v, new ez.Vec2(3, 3), 0.0001);

        // SetMul
        v.SetMul(new ez.Vec2(1, 2), 2);
        EZ_TEST_VEC2(v, new ez.Vec2(2, 4), 0.0001);

        // SetDiv
        v.SetDiv(new ez.Vec2(2, 4), 2);
        EZ_TEST_VEC2(v, new ez.Vec2(1, 2), 0.0001);

        // CreateRandomPointInCircle
        {
            let avg = new ez.Vec2();

            const uiNumSamples = 1000;
            for (let i = 0; i < uiNumSamples; ++i) {
                v = ez.Vec2.CreateRandomPointInCircle();

                EZ_TEST_BOOL(v.GetLength() <= 1.0);
                EZ_TEST_BOOL(!v.IsZero());

                avg.AddVec2(v);
            }

            avg.DivNumber(uiNumSamples);

            // the average point cloud center should be within at least 10% of the sphere's center
            // otherwise the points aren't equally distributed
            EZ_TEST_BOOL(avg.IsZero(0.1));
        }

        // CreateRandomDirection
        {
            let avg = new ez.Vec2();

            const uiNumSamples = 1000;
            for (let i = 0; i < uiNumSamples; ++i) {
                v = ez.Vec2.CreateRandomDirection();

                EZ_TEST_BOOL(v.IsNormalized());

                avg.AddVec2(v);
            }

            avg.DivNumber(uiNumSamples);

            // the average point cloud center should be within at least 10% of the sphere's center
            // otherwise the points aren't equally distributed
            EZ_TEST_BOOL(avg.IsZero(0.1));
        }
    }

    TestVec3(): void {
        // constructor
        let d = new ez.Vec3();
        EZ_TEST_FLOAT(d.x, 0, 0.001);
        EZ_TEST_FLOAT(d.y, 0, 0.001);
        EZ_TEST_FLOAT(d.z, 0, 0.001);

        // ZeroVector
        EZ_TEST_VEC3(new ez.Vec3(), ez.Vec3.ZeroVector(), 0.0001);

        let v = new ez.Vec3(1, 2, 3);
        EZ_TEST_FLOAT(v.x, 1, 0.001);
        EZ_TEST_FLOAT(v.y, 2, 0.001);
        EZ_TEST_FLOAT(v.z, 3, 0.001);

        // Clone
        EZ_TEST_VEC3(v.Clone(), v, 0.001);

        // CloneAsVec2
        EZ_TEST_VEC2(v.CloneAsVec2(), new ez.Vec2(1, 2), 0.001);

        // OneVector
        EZ_TEST_VEC3(new ez.Vec3(1, 1, 1), ez.Vec3.OneVector(), 0.0001);

        // UnitAxisX
        EZ_TEST_VEC3(new ez.Vec3(1, 0, 0), ez.Vec3.UnitAxisX(), 0.0001);

        // UnitAxisY
        EZ_TEST_VEC3(new ez.Vec3(0, 1, 0), ez.Vec3.UnitAxisY(), 0.0001);

        // UnitAxisZ
        EZ_TEST_VEC3(new ez.Vec3(0, 0, 1), ez.Vec3.UnitAxisZ(), 0.0001);

        // Set
        v.Set(4, 5, 6);
        EZ_TEST_FLOAT(v.x, 4, 0.001);
        EZ_TEST_FLOAT(v.y, 5, 0.001);
        EZ_TEST_FLOAT(v.z, 6, 0.001);

        // SetVec3
        let v2 = new ez.Vec3();
        v2.SetVec3(v);
        EZ_TEST_VEC3(v, v2, 0.0001);

        // SetAll
        v2.SetAll(7);
        EZ_TEST_FLOAT(v2.x, 7, 0.001);
        EZ_TEST_FLOAT(v2.y, 7, 0.001);
        EZ_TEST_FLOAT(v2.z, 7, 0.001);

        // SetZero
        v2.SetZero();
        EZ_TEST_FLOAT(v2.x, 0, 0.001);
        EZ_TEST_FLOAT(v2.y, 0, 0.001);
        EZ_TEST_FLOAT(v2.z, 0, 0.001);

        // GetLengthSquared
        EZ_TEST_FLOAT(v2.GetLengthSquared(), 0, 0.001);
        v2.SetAll(1);

        EZ_TEST_FLOAT(v2.GetLengthSquared(), 3, 0.001);

        // GetLength
        EZ_TEST_FLOAT(v2.GetLength(), Math.sqrt(3), 0.001);

        // GetLengthAndNormalize
        let l = v2.GetLengthAndNormalize();
        EZ_TEST_FLOAT(l, Math.sqrt(3), 0.001);
        EZ_TEST_FLOAT(v2.GetLength(), 1, 0.001);

        // IsNormalized
        EZ_TEST_BOOL(!v.IsNormalized());

        // Normalize
        v.Normalize();

        EZ_TEST_FLOAT(v.GetLength(), 1, 0.001);
        EZ_TEST_BOOL(v.IsNormalized());

        // GetNormalized
        v.Set(3, 0, 0);
        EZ_TEST_VEC3(v.GetNormalized(), ez.Vec3.UnitAxisX(), 0.0001);

        // NormalizeIfNotZero
        EZ_TEST_BOOL(v.NormalizeIfNotZero(ez.Vec3.UnitAxisZ(), 0.001));
        EZ_TEST_VEC3(v, ez.Vec3.UnitAxisX(), 0.0001);

        // IsZero
        EZ_TEST_BOOL(!v.IsZero());

        // SetZero
        v.SetZero();
        EZ_TEST_BOOL(v.IsZero());

        EZ_TEST_BOOL(!v.NormalizeIfNotZero(ez.Vec3.UnitAxisZ(), 0.001));
        EZ_TEST_VEC3(v, ez.Vec3.UnitAxisZ(), 0.0001);

        // GetNegated
        v.Set(1, 2, 3);
        EZ_TEST_VEC3(v.GetNegated(), new ez.Vec3(-1, -2, -3), 0.0001);
        EZ_TEST_VEC3(v, new ez.Vec3(1, 2, 3), 0.0001);

        // Negate
        v.Negate();
        EZ_TEST_VEC3(v, new ez.Vec3(-1, -2, -3), 0.0001);

        // AddVec3
        v.Set(2, 3, 4);
        v2.Set(5, 6, 7);
        v.AddVec3(v2);
        EZ_TEST_VEC3(v, new ez.Vec3(7, 9, 11), 0.0001);

        // SubVec3
        v.SubVec3(v2);
        EZ_TEST_VEC3(v, new ez.Vec3(2, 3, 4), 0.0001);

        // MulVec3
        v.MulVec3(v);
        EZ_TEST_VEC3(v, new ez.Vec3(4, 9, 16), 0.0001);

        // DivVec3
        v.DivVec3(new ez.Vec3(2, 3, 4));
        EZ_TEST_VEC3(v, new ez.Vec3(2, 3, 4), 0.0001);

        // MulNumber
        v.MulNumber(2);
        EZ_TEST_VEC3(v, new ez.Vec3(4, 6, 8), 0.0001);

        // DivNumber
        v.DivNumber(2);
        EZ_TEST_VEC3(v, new ez.Vec3(2, 3, 4), 0.0001);

        // IsIdentical
        EZ_TEST_BOOL(v.IsIdentical(v));
        EZ_TEST_BOOL(!v.IsIdentical(v2));

        // IsEqual
        EZ_TEST_BOOL(v.IsEqual(new ez.Vec3(2, 3, 4), 0.0001));
        EZ_TEST_BOOL(!v.IsEqual(new ez.Vec3(2, 3.5, 4), 0.0001));

        // Dot
        v.Set(2, 3, 4);
        v2.Set(3, 4, 5);
        EZ_TEST_FLOAT(v.Dot(v2), 38, 0.001);

        // CrossRH
        v.Set(1, 0, 0);
        v2.Set(0, 1, 0);
        EZ_TEST_VEC3(v.CrossRH(v2), new ez.Vec3(0, 0, 1), 0.001);

        // SetCrossRH
        let v3 = new ez.Vec3();
        v3.SetCrossRH(v, v2);
        EZ_TEST_VEC3(v3, new ez.Vec3(0, 0, 1), 0.001);

        // GetCompMin
        v.Set(2, 4, 6);
        v2.Set(1, 5, 7);
        EZ_TEST_VEC3(v.GetCompMin(v2), new ez.Vec3(1, 4, 6), 0.001);

        // GetCompMax
        v.Set(2, 4, 6);
        v2.Set(1, 5, 7);
        EZ_TEST_VEC3(v.GetCompMax(v2), new ez.Vec3(2, 5, 7), 0.001);

        // GetCompClamp
        EZ_TEST_VEC3(v.GetCompClamp(new ez.Vec3(3, 4, 5), new ez.Vec3(4, 5, 6)), new ez.Vec3(3, 4, 6), 0.001);

        // GetCompMul
        EZ_TEST_VEC3(v.GetCompMul(new ez.Vec3(2, 3, 4)), new ez.Vec3(4, 12, 24), 0.001);

        // GetCompDiv
        EZ_TEST_VEC3(v.GetCompDiv(new ez.Vec3(2, 4, 6)), ez.Vec3.OneVector(), 0.001);

        // GetAbs
        v.Set(-1, -2, -3);
        EZ_TEST_VEC3(v.GetAbs(), new ez.Vec3(1, 2, 3), 0.001);

        // SetAbs
        v2.SetAbs(v);
        EZ_TEST_VEC3(v2, new ez.Vec3(1, 2, 3), 0.001);

        // CalculateNormal
        EZ_TEST_BOOL(v.CalculateNormal(new ez.Vec3(-1, 0, 1), new ez.Vec3(1, 0, 1), new ez.Vec3(0, 0, -1)));
        EZ_TEST_VEC3(v, new ez.Vec3(0, 1, 0), 0.001);

        EZ_TEST_BOOL(v.CalculateNormal(new ez.Vec3(-1, 0, -1), new ez.Vec3(1, 0, -1), new ez.Vec3(0, 0, 1)));
        EZ_TEST_VEC3(v, new ez.Vec3(0, -1, 0), 0.001);

        EZ_TEST_BOOL(v.CalculateNormal(new ez.Vec3(-1, 0, 1), new ez.Vec3(1, 0, 1), new ez.Vec3(1, 0, 1)) == false);

        // MakeOrthogonalTo
        v.Set(1, 1, 0);
        v.MakeOrthogonalTo(new ez.Vec3(1, 0, 0));
        EZ_TEST_VEC3(v, new ez.Vec3(0, 1, 0), 0.001);

        v.Set(1, 1, 0);
        v.MakeOrthogonalTo(new ez.Vec3(0, 1, 0));
        EZ_TEST_VEC3(v, new ez.Vec3(1, 0, 0), 0.001);

        // GetOrthogonalVector
        for (let i = 1; i < 360; i += 3.0) {
            v.Set(i, i * 3, i * 7);
            EZ_TEST_FLOAT(v.GetOrthogonalVector().Dot(v), 0.0, 0.001);
        }

        // GetReflectedVector
        v.Set(1, 1, 0);
        v2 = v.GetReflectedVector(new ez.Vec3(0, -1, 0));
        EZ_TEST_VEC3(v2, new ez.Vec3(1, -1, 0), 0.0001);

        // SetAdd
        v.SetAdd(new ez.Vec3(1, 2, 3), new ez.Vec3(4, 5, 6));
        EZ_TEST_VEC3(v, new ez.Vec3(5, 7, 9), 0.0001);

        // SetSub
        v.SetSub(new ez.Vec3(4, 5, 6), new ez.Vec3(1, 2, 3));
        EZ_TEST_VEC3(v, new ez.Vec3(3, 3, 3), 0.0001);

        // SetMul
        v.SetMul(new ez.Vec3(1, 2, 3), 2);
        EZ_TEST_VEC3(v, new ez.Vec3(2, 4, 6), 0.0001);

        // SetDiv
        v.SetDiv(new ez.Vec3(2, 4, 6), 2);
        EZ_TEST_VEC3(v, new ez.Vec3(1, 2, 3), 0.0001);

        // SetLength
        v.Set(0, 2, 0);
        EZ_TEST_BOOL(v.SetLength(5, 0.001));
        EZ_TEST_VEC3(v, new ez.Vec3(0, 5, 0), 0.0001);

        // CreateRandomPointInSphere
        {
            let avg = new ez.Vec3();

            const uiNumSamples = 1000;
            for (let i = 0; i < uiNumSamples; ++i) {
                v = ez.Vec3.CreateRandomPointInSphere();

                EZ_TEST_BOOL(v.GetLength() <= 1.0);
                EZ_TEST_BOOL(!v.IsZero());

                avg.AddVec3(v);
            }

            avg.DivNumber(uiNumSamples);

            // the average point cloud center should be within at least 10% of the sphere's center
            // otherwise the points aren't equally distributed
            EZ_TEST_BOOL(avg.IsZero(0.1));
        }

        // CreateRandomDirection
        {
            let avg = new ez.Vec3();

            const uiNumSamples = 1000;
            for (let i = 0; i < uiNumSamples; ++i) {
                v = ez.Vec3.CreateRandomDirection();

                EZ_TEST_BOOL(v.IsNormalized());

                avg.AddVec3(v);
            }

            avg.DivNumber(uiNumSamples);

            // the average point cloud center should be within at least 10% of the sphere's center
            // otherwise the points aren't equally distributed
            EZ_TEST_BOOL(avg.IsZero(0.1));
        }
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "TestVec3") {

            this.TestVec3();

            msg.Message = "done";
        }

        if (msg.Message == "TestVec2") {

            this.TestVec2();

            msg.Message = "done";
        }
    }

}

