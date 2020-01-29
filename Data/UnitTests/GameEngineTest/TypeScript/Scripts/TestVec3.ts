import ez = require("TypeScript/ez")
import EZ_TEST = require("./TestFramework")

export class TestVec3 extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgGenericEvent, "OnMsgGenericEvent");
    }

    ExecuteTests(): void {
        // constructor
        let d = new ez.Vec3();
        EZ_TEST.FLOAT(d.x, 0, 0.001);
        EZ_TEST.FLOAT(d.y, 0, 0.001);
        EZ_TEST.FLOAT(d.z, 0, 0.001);

        // ZeroVector
        EZ_TEST.VEC3(new ez.Vec3(), ez.Vec3.ZeroVector(), 0.0001);

        let v = new ez.Vec3(1, 2, 3);
        EZ_TEST.FLOAT(v.x, 1, 0.001);
        EZ_TEST.FLOAT(v.y, 2, 0.001);
        EZ_TEST.FLOAT(v.z, 3, 0.001);

        // Clone
        EZ_TEST.VEC3(v.Clone(), v, 0.001);

        // CloneAsVec2
        EZ_TEST.VEC2(v.CloneAsVec2(), new ez.Vec2(1, 2), 0.001);

        // OneVector
        EZ_TEST.VEC3(new ez.Vec3(1, 1, 1), ez.Vec3.OneVector(), 0.0001);

        // UnitAxisX
        EZ_TEST.VEC3(new ez.Vec3(1, 0, 0), ez.Vec3.UnitAxisX(), 0.0001);

        // UnitAxisY
        EZ_TEST.VEC3(new ez.Vec3(0, 1, 0), ez.Vec3.UnitAxisY(), 0.0001);

        // UnitAxisZ
        EZ_TEST.VEC3(new ez.Vec3(0, 0, 1), ez.Vec3.UnitAxisZ(), 0.0001);

        // Set
        v.Set(4, 5, 6);
        EZ_TEST.FLOAT(v.x, 4, 0.001);
        EZ_TEST.FLOAT(v.y, 5, 0.001);
        EZ_TEST.FLOAT(v.z, 6, 0.001);

        // SetVec3
        let v2 = new ez.Vec3();
        v2.SetVec3(v);
        EZ_TEST.VEC3(v, v2, 0.0001);

        // SetAll
        v2.SetAll(7);
        EZ_TEST.FLOAT(v2.x, 7, 0.001);
        EZ_TEST.FLOAT(v2.y, 7, 0.001);
        EZ_TEST.FLOAT(v2.z, 7, 0.001);

        // SetZero
        v2.SetZero();
        EZ_TEST.FLOAT(v2.x, 0, 0.001);
        EZ_TEST.FLOAT(v2.y, 0, 0.001);
        EZ_TEST.FLOAT(v2.z, 0, 0.001);

        // GetLengthSquared
        EZ_TEST.FLOAT(v2.GetLengthSquared(), 0, 0.001);
        v2.SetAll(1);

        EZ_TEST.FLOAT(v2.GetLengthSquared(), 3, 0.001);

        // GetLength
        EZ_TEST.FLOAT(v2.GetLength(), Math.sqrt(3), 0.001);

        // GetLengthAndNormalize
        let l = v2.GetLengthAndNormalize();
        EZ_TEST.FLOAT(l, Math.sqrt(3), 0.001);
        EZ_TEST.FLOAT(v2.GetLength(), 1, 0.001);

        // IsNormalized
        EZ_TEST.BOOL(!v.IsNormalized());

        // Normalize
        v.Normalize();

        EZ_TEST.FLOAT(v.GetLength(), 1, 0.001);
        EZ_TEST.BOOL(v.IsNormalized());

        // GetNormalized
        v.Set(3, 0, 0);
        EZ_TEST.VEC3(v.GetNormalized(), ez.Vec3.UnitAxisX(), 0.0001);

        // NormalizeIfNotZero
        EZ_TEST.BOOL(v.NormalizeIfNotZero(ez.Vec3.UnitAxisZ(), 0.001));
        EZ_TEST.VEC3(v, ez.Vec3.UnitAxisX(), 0.0001);

        // IsZero
        EZ_TEST.BOOL(!v.IsZero());

        // SetZero
        v.SetZero();
        EZ_TEST.BOOL(v.IsZero());

        EZ_TEST.BOOL(!v.NormalizeIfNotZero(ez.Vec3.UnitAxisZ(), 0.001));
        EZ_TEST.VEC3(v, ez.Vec3.UnitAxisZ(), 0.0001);

        // GetNegated
        v.Set(1, 2, 3);
        EZ_TEST.VEC3(v.GetNegated(), new ez.Vec3(-1, -2, -3), 0.0001);
        EZ_TEST.VEC3(v, new ez.Vec3(1, 2, 3), 0.0001);

        // Negate
        v.Negate();
        EZ_TEST.VEC3(v, new ez.Vec3(-1, -2, -3), 0.0001);

        // AddVec3
        v.Set(2, 3, 4);
        v2.Set(5, 6, 7);
        v.AddVec3(v2);
        EZ_TEST.VEC3(v, new ez.Vec3(7, 9, 11), 0.0001);

        // SubVec3
        v.SubVec3(v2);
        EZ_TEST.VEC3(v, new ez.Vec3(2, 3, 4), 0.0001);

        // MulVec3
        v.MulVec3(v);
        EZ_TEST.VEC3(v, new ez.Vec3(4, 9, 16), 0.0001);

        // DivVec3
        v.DivVec3(new ez.Vec3(2, 3, 4));
        EZ_TEST.VEC3(v, new ez.Vec3(2, 3, 4), 0.0001);

        // MulNumber
        v.MulNumber(2);
        EZ_TEST.VEC3(v, new ez.Vec3(4, 6, 8), 0.0001);

        // DivNumber
        v.DivNumber(2);
        EZ_TEST.VEC3(v, new ez.Vec3(2, 3, 4), 0.0001);

        // IsIdentical
        EZ_TEST.BOOL(v.IsIdentical(v));
        EZ_TEST.BOOL(!v.IsIdentical(v2));

        // IsEqual
        EZ_TEST.BOOL(v.IsEqual(new ez.Vec3(2, 3, 4), 0.0001));
        EZ_TEST.BOOL(!v.IsEqual(new ez.Vec3(2, 3.5, 4), 0.0001));

        // Dot
        v.Set(2, 3, 4);
        v2.Set(3, 4, 5);
        EZ_TEST.FLOAT(v.Dot(v2), 38, 0.001);

        // CrossRH
        v.Set(1, 0, 0);
        v2.Set(0, 1, 0);
        EZ_TEST.VEC3(v.CrossRH(v2), new ez.Vec3(0, 0, 1), 0.001);

        // SetCrossRH
        let v3 = new ez.Vec3();
        v3.SetCrossRH(v, v2);
        EZ_TEST.VEC3(v3, new ez.Vec3(0, 0, 1), 0.001);

        // GetCompMin
        v.Set(2, 4, 6);
        v2.Set(1, 5, 7);
        EZ_TEST.VEC3(v.GetCompMin(v2), new ez.Vec3(1, 4, 6), 0.001);

        // GetCompMax
        v.Set(2, 4, 6);
        v2.Set(1, 5, 7);
        EZ_TEST.VEC3(v.GetCompMax(v2), new ez.Vec3(2, 5, 7), 0.001);

        // GetCompClamp
        EZ_TEST.VEC3(v.GetCompClamp(new ez.Vec3(3, 4, 5), new ez.Vec3(4, 5, 6)), new ez.Vec3(3, 4, 6), 0.001);

        // GetCompMul
        EZ_TEST.VEC3(v.GetCompMul(new ez.Vec3(2, 3, 4)), new ez.Vec3(4, 12, 24), 0.001);

        // GetCompDiv
        EZ_TEST.VEC3(v.GetCompDiv(new ez.Vec3(2, 4, 6)), ez.Vec3.OneVector(), 0.001);

        // GetAbs
        v.Set(-1, -2, -3);
        EZ_TEST.VEC3(v.GetAbs(), new ez.Vec3(1, 2, 3), 0.001);

        // SetAbs
        v2.SetAbs(v);
        EZ_TEST.VEC3(v2, new ez.Vec3(1, 2, 3), 0.001);

        // CalculateNormal
        EZ_TEST.BOOL(v.CalculateNormal(new ez.Vec3(-1, 0, 1), new ez.Vec3(1, 0, 1), new ez.Vec3(0, 0, -1)));
        EZ_TEST.VEC3(v, new ez.Vec3(0, 1, 0), 0.001);

        EZ_TEST.BOOL(v.CalculateNormal(new ez.Vec3(-1, 0, -1), new ez.Vec3(1, 0, -1), new ez.Vec3(0, 0, 1)));
        EZ_TEST.VEC3(v, new ez.Vec3(0, -1, 0), 0.001);

        EZ_TEST.BOOL(v.CalculateNormal(new ez.Vec3(-1, 0, 1), new ez.Vec3(1, 0, 1), new ez.Vec3(1, 0, 1)) == false);

        // MakeOrthogonalTo
        v.Set(1, 1, 0);
        v.MakeOrthogonalTo(new ez.Vec3(1, 0, 0));
        EZ_TEST.VEC3(v, new ez.Vec3(0, 1, 0), 0.001);

        v.Set(1, 1, 0);
        v.MakeOrthogonalTo(new ez.Vec3(0, 1, 0));
        EZ_TEST.VEC3(v, new ez.Vec3(1, 0, 0), 0.001);

        // GetOrthogonalVector
        for (let i = 1; i < 360; i += 3.0) {
            v.Set(i, i * 3, i * 7);
            EZ_TEST.FLOAT(v.GetOrthogonalVector().Dot(v), 0.0, 0.001);
        }

        // GetReflectedVector
        v.Set(1, 1, 0);
        v2 = v.GetReflectedVector(new ez.Vec3(0, -1, 0));
        EZ_TEST.VEC3(v2, new ez.Vec3(1, -1, 0), 0.0001);

        // SetAdd
        v.SetAdd(new ez.Vec3(1, 2, 3), new ez.Vec3(4, 5, 6));
        EZ_TEST.VEC3(v, new ez.Vec3(5, 7, 9), 0.0001);

        // SetSub
        v.SetSub(new ez.Vec3(4, 5, 6), new ez.Vec3(1, 2, 3));
        EZ_TEST.VEC3(v, new ez.Vec3(3, 3, 3), 0.0001);

        // SetMul
        v.SetMul(new ez.Vec3(1, 2, 3), 2);
        EZ_TEST.VEC3(v, new ez.Vec3(2, 4, 6), 0.0001);

        // SetDiv
        v.SetDiv(new ez.Vec3(2, 4, 6), 2);
        EZ_TEST.VEC3(v, new ez.Vec3(1, 2, 3), 0.0001);

        // SetLength
        v.Set(0, 2, 0);
        EZ_TEST.BOOL(v.SetLength(5, 0.001));
        EZ_TEST.VEC3(v, new ez.Vec3(0, 5, 0), 0.0001);

        // CreateRandomPointInSphere
        {
            let avg = new ez.Vec3();

            const uiNumSamples = 1000;
            for (let i = 0; i < uiNumSamples; ++i) {
                v = ez.Vec3.CreateRandomPointInSphere();

                EZ_TEST.BOOL(v.GetLength() <= 1.0);
                EZ_TEST.BOOL(!v.IsZero());

                avg.AddVec3(v);
            }

            avg.DivNumber(uiNumSamples);

            // the average point cloud center should be within at least 10% of the sphere's center
            // otherwise the points aren't equally distributed
            EZ_TEST.BOOL(avg.IsZero(0.1));
        }

        // CreateRandomDirection
        {
            let avg = new ez.Vec3();

            const uiNumSamples = 1000;
            for (let i = 0; i < uiNumSamples; ++i) {
                v = ez.Vec3.CreateRandomDirection();

                EZ_TEST.BOOL(v.IsNormalized());

                avg.AddVec3(v);
            }

            avg.DivNumber(uiNumSamples);

            // the average point cloud center should be within at least 10% of the sphere's center
            // otherwise the points aren't equally distributed
            EZ_TEST.BOOL(avg.IsZero(0.1));
        }
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "TestVec3") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

