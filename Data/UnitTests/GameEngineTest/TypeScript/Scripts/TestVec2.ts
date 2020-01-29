import ez = require("TypeScript/ez")
import EZ_TEST = require("./TestFramework")

export class TestVec2 extends ez.TypescriptComponent {

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
        let d = new ez.Vec2();
        EZ_TEST.FLOAT(d.x, 0, 0.001);
        EZ_TEST.FLOAT(d.y, 0, 0.001);

        // ZeroVector
        EZ_TEST.VEC2(new ez.Vec2(), ez.Vec2.ZeroVector(), 0.0001);

        let v = new ez.Vec2(1, 2);
        EZ_TEST.FLOAT(v.x, 1, 0.001);
        EZ_TEST.FLOAT(v.y, 2, 0.001);

        // Clone
        EZ_TEST.VEC2(v.Clone(), v, 0.001);

        // TODO: CloneAsVec3
        //EZ_TEST.VEC3(v.CloneAsVec3(3), new ez.Vec3(1, 2, 3), 0.001);

        // OneVector
        EZ_TEST.VEC2(new ez.Vec2(1, 1), ez.Vec2.OneVector(), 0.0001);

        // UnitAxisX
        EZ_TEST.VEC2(new ez.Vec2(1, 0), ez.Vec2.UnitAxisX(), 0.0001);

        // UnitAxisY
        EZ_TEST.VEC2(new ez.Vec2(0, 1), ez.Vec2.UnitAxisY(), 0.0001);

        // Set
        v.Set(4, 5);
        EZ_TEST.FLOAT(v.x, 4, 0.001);
        EZ_TEST.FLOAT(v.y, 5, 0.001);

        // SetVec2
        let v2 = new ez.Vec2();
        v2.SetVec2(v);
        EZ_TEST.VEC2(v, v2, 0.0001);

        // SetAll
        v2.SetAll(7);
        EZ_TEST.FLOAT(v2.x, 7, 0.001);
        EZ_TEST.FLOAT(v2.y, 7, 0.001);

        // SetZero
        v2.SetZero();
        EZ_TEST.FLOAT(v2.x, 0, 0.001);
        EZ_TEST.FLOAT(v2.y, 0, 0.001);

        // GetLengthSquared
        EZ_TEST.FLOAT(v2.GetLengthSquared(), 0, 0.001);
        v2.SetAll(1);

        EZ_TEST.FLOAT(v2.GetLengthSquared(), 2, 0.001);

        // GetLength
        EZ_TEST.FLOAT(v2.GetLength(), Math.sqrt(2), 0.001);

        // GetLengthAndNormalize
        let l = v2.GetLengthAndNormalize();
        EZ_TEST.FLOAT(l, Math.sqrt(2), 0.001);
        EZ_TEST.FLOAT(v2.GetLength(), 1, 0.001);

        // IsNormalized
        EZ_TEST.BOOL(!v.IsNormalized());

        // Normalize
        v.Normalize();

        EZ_TEST.FLOAT(v.GetLength(), 1, 0.001);
        EZ_TEST.BOOL(v.IsNormalized());


        // GetNormalized
        v.Set(3, 0);
        EZ_TEST.VEC2(v.GetNormalized(), ez.Vec2.UnitAxisX(), 0.0001);

        // NormalizeIfNotZero
        EZ_TEST.BOOL(v.NormalizeIfNotZero(ez.Vec2.UnitAxisY(), 0.001));
        EZ_TEST.VEC2(v, ez.Vec2.UnitAxisX(), 0.0001);

        // IsZero
        EZ_TEST.BOOL(!v.IsZero());

        // SetZero
        v.SetZero();
        EZ_TEST.BOOL(v.IsZero());

        EZ_TEST.BOOL(!v.NormalizeIfNotZero(ez.Vec2.UnitAxisY(), 0.001));
        EZ_TEST.VEC2(v, ez.Vec2.UnitAxisY(), 0.0001);

        // GetNegated
        v.Set(1, 2);
        EZ_TEST.VEC2(v.GetNegated(), new ez.Vec2(-1, -2), 0.0001);
        EZ_TEST.VEC2(v, new ez.Vec2(1, 2), 0.0001);

        // Negate
        v.Negate();
        EZ_TEST.VEC2(v, new ez.Vec2(-1, -2), 0.0001);

        // AddVec2
        v.Set(2, 3);
        v2.Set(5, 6);
        v.AddVec2(v2);
        EZ_TEST.VEC2(v, new ez.Vec2(7, 9), 0.0001);

        // SubVec2
        v.SubVec2(v2);
        EZ_TEST.VEC2(v, new ez.Vec2(2, 3), 0.0001);

        // MulVec2
        v.MulVec2(v);
        EZ_TEST.VEC2(v, new ez.Vec2(4, 9), 0.0001);

        // DivVec2
        v.DivVec2(new ez.Vec2(2, 3));
        EZ_TEST.VEC2(v, new ez.Vec2(2, 3), 0.0001);

        // MulNumber
        v.MulNumber(2);
        EZ_TEST.VEC2(v, new ez.Vec2(4, 6), 0.0001);

        // DivNumber
        v.DivNumber(2);
        EZ_TEST.VEC2(v, new ez.Vec2(2, 3), 0.0001);

        // IsIdentical
        EZ_TEST.BOOL(v.IsIdentical(v));
        EZ_TEST.BOOL(!v.IsIdentical(v2));

        // IsEqual
        EZ_TEST.BOOL(v.IsEqual(new ez.Vec2(2, 3), 0.0001));
        EZ_TEST.BOOL(!v.IsEqual(new ez.Vec2(2, 3.5), 0.0001));

        // Dot
        v.Set(2, 3);
        v2.Set(3, 4);
        EZ_TEST.FLOAT(v.Dot(v2), 18, 0.001);

        // GetCompMin
        v.Set(2, 4);
        v2.Set(1, 5);
        EZ_TEST.VEC2(v.GetCompMin(v2), new ez.Vec2(1, 4), 0.001);

        // GetCompMax
        v.Set(2, 4);
        v2.Set(1, 5);
        EZ_TEST.VEC2(v.GetCompMax(v2), new ez.Vec2(2, 5), 0.001);

        // GetCompClamp
        EZ_TEST.VEC2(v.GetCompClamp(new ez.Vec2(3, 4), new ez.Vec2(4, 5)), new ez.Vec2(3, 4), 0.001);

        // GetCompMul
        EZ_TEST.VEC2(v.GetCompMul(new ez.Vec2(2, 3)), new ez.Vec2(4, 12), 0.001);

        // GetCompDiv
        EZ_TEST.VEC2(v.GetCompDiv(new ez.Vec2(2, 4)), ez.Vec2.OneVector(), 0.001);

        // GetAbs
        v.Set(-1, -2);
        EZ_TEST.VEC2(v.GetAbs(), new ez.Vec2(1, 2), 0.001);

        // SetAbs
        v2.SetAbs(v);
        EZ_TEST.VEC2(v2, new ez.Vec2(1, 2), 0.001);

        // GetReflectedVector
        v.Set(1, 1);
        v2 = v.GetReflectedVector(new ez.Vec2(0, -1));
        EZ_TEST.VEC2(v2, new ez.Vec2(1, -1), 0.0001);

        // SetAdd
        v.SetAdd(new ez.Vec2(1, 2), new ez.Vec2(4, 5));
        EZ_TEST.VEC2(v, new ez.Vec2(5, 7), 0.0001);

        // SetSub
        v.SetSub(new ez.Vec2(4, 5), new ez.Vec2(1, 2));
        EZ_TEST.VEC2(v, new ez.Vec2(3, 3), 0.0001);

        // SetMul
        v.SetMul(new ez.Vec2(1, 2), 2);
        EZ_TEST.VEC2(v, new ez.Vec2(2, 4), 0.0001);

        // SetDiv
        v.SetDiv(new ez.Vec2(2, 4), 2);
        EZ_TEST.VEC2(v, new ez.Vec2(1, 2), 0.0001);

        // CreateRandomPointInCircle
        {
            let avg = new ez.Vec2();

            const uiNumSamples = 1000;
            for (let i = 0; i < uiNumSamples; ++i) {
                v = ez.Vec2.CreateRandomPointInCircle();

                EZ_TEST.BOOL(v.GetLength() <= 1.0);
                EZ_TEST.BOOL(!v.IsZero());

                avg.AddVec2(v);
            }

            avg.DivNumber(uiNumSamples);

            // the average point cloud center should be within at least 10% of the sphere's center
            // otherwise the points aren't equally distributed
            EZ_TEST.BOOL(avg.IsZero(0.1));
        }

        // CreateRandomDirection
        {
            let avg = new ez.Vec2();

            const uiNumSamples = 1000;
            for (let i = 0; i < uiNumSamples; ++i) {
                v = ez.Vec2.CreateRandomDirection();

                EZ_TEST.BOOL(v.IsNormalized());

                avg.AddVec2(v);
            }

            avg.DivNumber(uiNumSamples);

            // the average point cloud center should be within at least 10% of the sphere's center
            // otherwise the points aren't equally distributed
            EZ_TEST.BOOL(avg.IsZero(0.1));
        }
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "TestVec2") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

