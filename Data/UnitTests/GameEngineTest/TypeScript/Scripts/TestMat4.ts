import ez = require("TypeScript/ez")
import EZ_TEST = require("./TestFramework")

function mul(m: ez.Mat4, v: ez.Vec3): ez.Vec3 {
    let r = v.Clone();
    m.TransformPosition(r);
    return r;
}

export class TestMat4 extends ez.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        ez.TypescriptComponent.RegisterMessageHandler(ez.MsgGenericEvent, "OnMsgGenericEvent");
    }

    ExecuteTests(): void {

        // Constructor (default)
        {
            let m = new ez.Mat4();

            EZ_TEST.FLOAT(m.GetElement(0, 0), 1, 0.001);
            EZ_TEST.FLOAT(m.GetElement(1, 0), 0, 0.001);
            EZ_TEST.FLOAT(m.GetElement(2, 0), 0, 0.001);
            EZ_TEST.FLOAT(m.GetElement(3, 0), 0, 0.001);
            EZ_TEST.FLOAT(m.GetElement(0, 1), 0, 0.001);
            EZ_TEST.FLOAT(m.GetElement(1, 1), 1, 0.001);
            EZ_TEST.FLOAT(m.GetElement(2, 1), 0, 0.001);
            EZ_TEST.FLOAT(m.GetElement(3, 1), 0, 0.001);
            EZ_TEST.FLOAT(m.GetElement(0, 2), 0, 0.001);
            EZ_TEST.FLOAT(m.GetElement(1, 2), 0, 0.001);
            EZ_TEST.FLOAT(m.GetElement(2, 2), 1, 0.001);
            EZ_TEST.FLOAT(m.GetElement(3, 2), 0, 0.001);
            EZ_TEST.FLOAT(m.GetElement(0, 3), 0, 0.001);
            EZ_TEST.FLOAT(m.GetElement(1, 3), 0, 0.001);
            EZ_TEST.FLOAT(m.GetElement(2, 3), 0, 0.001);
            EZ_TEST.FLOAT(m.GetElement(3, 3), 1, 0.001);

            EZ_TEST.BOOL(m.IsIdentity());
        }

        // Constructor (Elements)
        {
            let m = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            EZ_TEST.FLOAT(m.GetElement(0, 0), 1, 0.001);
            EZ_TEST.FLOAT(m.GetElement(1, 0), 2, 0.001);
            EZ_TEST.FLOAT(m.GetElement(2, 0), 3, 0.001);
            EZ_TEST.FLOAT(m.GetElement(3, 0), 4, 0.001);
            EZ_TEST.FLOAT(m.GetElement(0, 1), 5, 0.001);
            EZ_TEST.FLOAT(m.GetElement(1, 1), 6, 0.001);
            EZ_TEST.FLOAT(m.GetElement(2, 1), 7, 0.001);
            EZ_TEST.FLOAT(m.GetElement(3, 1), 8, 0.001);
            EZ_TEST.FLOAT(m.GetElement(0, 2), 9, 0.001);
            EZ_TEST.FLOAT(m.GetElement(1, 2), 10, 0.001);
            EZ_TEST.FLOAT(m.GetElement(2, 2), 11, 0.001);
            EZ_TEST.FLOAT(m.GetElement(3, 2), 12, 0.001);
            EZ_TEST.FLOAT(m.GetElement(0, 3), 13, 0.001);
            EZ_TEST.FLOAT(m.GetElement(1, 3), 14, 0.001);
            EZ_TEST.FLOAT(m.GetElement(2, 3), 15, 0.001);
            EZ_TEST.FLOAT(m.GetElement(3, 3), 16, 0.001);
        }

        // Clone
        {
            let m0 = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
            let m = m0.Clone();

            EZ_TEST.FLOAT(m.GetElement(0, 0), 1, 0.001);
            EZ_TEST.FLOAT(m.GetElement(1, 0), 2, 0.001);
            EZ_TEST.FLOAT(m.GetElement(2, 0), 3, 0.001);
            EZ_TEST.FLOAT(m.GetElement(3, 0), 4, 0.001);
            EZ_TEST.FLOAT(m.GetElement(0, 1), 5, 0.001);
            EZ_TEST.FLOAT(m.GetElement(1, 1), 6, 0.001);
            EZ_TEST.FLOAT(m.GetElement(2, 1), 7, 0.001);
            EZ_TEST.FLOAT(m.GetElement(3, 1), 8, 0.001);
            EZ_TEST.FLOAT(m.GetElement(0, 2), 9, 0.001);
            EZ_TEST.FLOAT(m.GetElement(1, 2), 10, 0.001);
            EZ_TEST.FLOAT(m.GetElement(2, 2), 11, 0.001);
            EZ_TEST.FLOAT(m.GetElement(3, 2), 12, 0.001);
            EZ_TEST.FLOAT(m.GetElement(0, 3), 13, 0.001);
            EZ_TEST.FLOAT(m.GetElement(1, 3), 14, 0.001);
            EZ_TEST.FLOAT(m.GetElement(2, 3), 15, 0.001);
            EZ_TEST.FLOAT(m.GetElement(3, 3), 16, 0.001);
        }

        // CloneAsMat3
        {
            let m0 = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
            let m = m0.CloneAsMat3();

            EZ_TEST.FLOAT(m.GetElement(0, 0), 1, 0.001);
            EZ_TEST.FLOAT(m.GetElement(1, 0), 2, 0.001);
            EZ_TEST.FLOAT(m.GetElement(2, 0), 3, 0.001);
            EZ_TEST.FLOAT(m.GetElement(0, 1), 5, 0.001);
            EZ_TEST.FLOAT(m.GetElement(1, 1), 6, 0.001);
            EZ_TEST.FLOAT(m.GetElement(2, 1), 7, 0.001);
            EZ_TEST.FLOAT(m.GetElement(0, 2), 9, 0.001);
            EZ_TEST.FLOAT(m.GetElement(1, 2), 10, 0.001);
            EZ_TEST.FLOAT(m.GetElement(2, 2), 11, 0.001);
        }

        // SetMat4
        {
            let m0 = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
            let m = new ez.Mat4();
            m.SetMat4(m0);

            EZ_TEST.FLOAT(m.GetElement(0, 0), 1, 0.001);
            EZ_TEST.FLOAT(m.GetElement(1, 0), 2, 0.001);
            EZ_TEST.FLOAT(m.GetElement(2, 0), 3, 0.001);
            EZ_TEST.FLOAT(m.GetElement(3, 0), 4, 0.001);
            EZ_TEST.FLOAT(m.GetElement(0, 1), 5, 0.001);
            EZ_TEST.FLOAT(m.GetElement(1, 1), 6, 0.001);
            EZ_TEST.FLOAT(m.GetElement(2, 1), 7, 0.001);
            EZ_TEST.FLOAT(m.GetElement(3, 1), 8, 0.001);
            EZ_TEST.FLOAT(m.GetElement(0, 2), 9, 0.001);
            EZ_TEST.FLOAT(m.GetElement(1, 2), 10, 0.001);
            EZ_TEST.FLOAT(m.GetElement(2, 2), 11, 0.001);
            EZ_TEST.FLOAT(m.GetElement(3, 2), 12, 0.001);
            EZ_TEST.FLOAT(m.GetElement(0, 3), 13, 0.001);
            EZ_TEST.FLOAT(m.GetElement(1, 3), 14, 0.001);
            EZ_TEST.FLOAT(m.GetElement(2, 3), 15, 0.001);
            EZ_TEST.FLOAT(m.GetElement(3, 3), 16, 0.001);
        }

        // SetElement
        {
            let m = ez.Mat4.ZeroMatrix();

            m.SetElement(0, 0, 1);
            m.SetElement(1, 0, 2);
            m.SetElement(2, 0, 3);
            m.SetElement(3, 0, 4);
            m.SetElement(0, 1, 5);
            m.SetElement(1, 1, 6);
            m.SetElement(2, 1, 7);
            m.SetElement(3, 1, 8);
            m.SetElement(0, 2, 9);
            m.SetElement(1, 2, 10);
            m.SetElement(2, 2, 11);
            m.SetElement(3, 2, 12);
            m.SetElement(0, 3, 13);
            m.SetElement(1, 3, 14);
            m.SetElement(2, 3, 15);
            m.SetElement(3, 3, 16);

            let m0 = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
            EZ_TEST.BOOL(m.IsIdentical(m0));
        }

        // SetElements
        {
            let m = ez.Mat4.ZeroMatrix();
            m.SetElements(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            let m0 = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
            EZ_TEST.BOOL(m.IsIdentical(m0));
        }

        // SetFromArray
        {
            const data = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16];

            {
                let m = new ez.Mat4();
                m.SetFromArray(data, true);

                EZ_TEST.BOOL(m.m_ElementsCM[0] == 1.0 && m.m_ElementsCM[1] == 2.0 && m.m_ElementsCM[2] == 3.0 && m.m_ElementsCM[3] == 4.0 &&
                    m.m_ElementsCM[4] == 5.0 && m.m_ElementsCM[5] == 6.0 && m.m_ElementsCM[6] == 7.0 && m.m_ElementsCM[7] == 8.0 &&
                    m.m_ElementsCM[8] == 9.0 && m.m_ElementsCM[9] == 10.0 && m.m_ElementsCM[10] == 11.0 &&
                    m.m_ElementsCM[11] == 12.0 && m.m_ElementsCM[12] == 13.0 && m.m_ElementsCM[13] == 14.0 &&
                    m.m_ElementsCM[14] == 15.0 && m.m_ElementsCM[15] == 16.0);
            }

            {
                let m = new ez.Mat4();
                m.SetFromArray(data, false);

                EZ_TEST.BOOL(m.m_ElementsCM[0] == 1.0 && m.m_ElementsCM[1] == 5.0 && m.m_ElementsCM[2] == 9.0 && m.m_ElementsCM[3] == 13.0 &&
                    m.m_ElementsCM[4] == 2.0 && m.m_ElementsCM[5] == 6.0 && m.m_ElementsCM[6] == 10.0 && m.m_ElementsCM[7] == 14.0 &&
                    m.m_ElementsCM[8] == 3.0 && m.m_ElementsCM[9] == 7.0 && m.m_ElementsCM[10] == 11.0 &&
                    m.m_ElementsCM[11] == 15.0 && m.m_ElementsCM[12] == 4.0 && m.m_ElementsCM[13] == 8.0 &&
                    m.m_ElementsCM[14] == 12.0 && m.m_ElementsCM[15] == 16.0);
            }
        }

        // SetTransformationMatrix
        {
            let mr = new ez.Mat3(1, 2, 3, 4, 5, 6, 7, 8, 9);
            let vt = new ez.Vec3(10, 11, 12);

            let m = new ez.Mat4();
            m.SetTransformationMatrix(mr, vt);

            EZ_TEST.FLOAT(m.GetElement(0, 0), 1, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 0), 2, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 0), 3, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 0), 10, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 1), 4, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 1), 5, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 1), 6, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 1), 11, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 2), 7, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 2), 8, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 2), 9, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 2), 12, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 3), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 3), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 3), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 3), 1, 0);
        }

        // GetAsArray
        {
            let m = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            let data = m.GetAsArray(true);
            EZ_TEST.FLOAT(data[0], 1, 0.0001);
            EZ_TEST.FLOAT(data[1], 5, 0.0001);
            EZ_TEST.FLOAT(data[2], 9, 0.0001);
            EZ_TEST.FLOAT(data[3], 13, 0.0001);
            EZ_TEST.FLOAT(data[4], 2, 0.0001);
            EZ_TEST.FLOAT(data[5], 6, 0.0001);
            EZ_TEST.FLOAT(data[6], 10, 0.0001);
            EZ_TEST.FLOAT(data[7], 14, 0.0001);
            EZ_TEST.FLOAT(data[8], 3, 0.0001);
            EZ_TEST.FLOAT(data[9], 7, 0.0001);
            EZ_TEST.FLOAT(data[10], 11, 0.0001);
            EZ_TEST.FLOAT(data[11], 15, 0.0001);
            EZ_TEST.FLOAT(data[12], 4, 0.0001);
            EZ_TEST.FLOAT(data[13], 8, 0.0001);
            EZ_TEST.FLOAT(data[14], 12, 0.0001);
            EZ_TEST.FLOAT(data[15], 16, 0.0001);

            data = m.GetAsArray(false);
            EZ_TEST.FLOAT(data[0], 1, 0.0001);
            EZ_TEST.FLOAT(data[1], 2, 0.0001);
            EZ_TEST.FLOAT(data[2], 3, 0.0001);
            EZ_TEST.FLOAT(data[3], 4, 0.0001);
            EZ_TEST.FLOAT(data[4], 5, 0.0001);
            EZ_TEST.FLOAT(data[5], 6, 0.0001);
            EZ_TEST.FLOAT(data[6], 7, 0.0001);
            EZ_TEST.FLOAT(data[7], 8, 0.0001);
            EZ_TEST.FLOAT(data[8], 9, 0.0001);
            EZ_TEST.FLOAT(data[9], 10, 0.0001);
            EZ_TEST.FLOAT(data[10], 11, 0.0001);
            EZ_TEST.FLOAT(data[11], 12, 0.0001);
            EZ_TEST.FLOAT(data[12], 13, 0.0001);
            EZ_TEST.FLOAT(data[13], 14, 0.0001);
            EZ_TEST.FLOAT(data[14], 15, 0.0001);
            EZ_TEST.FLOAT(data[15], 16, 0.0001);
        }

        // SetZero
        {
            let m = new ez.Mat4();
            m.SetZero();

            for (let i = 0; i < 16; ++i)
                EZ_TEST.FLOAT(m.m_ElementsCM[i], 0.0, 0.0);
        }

        // SetIdentity
        {
            let m = new ez.Mat4();
            m.SetIdentity();

            EZ_TEST.FLOAT(m.GetElement(0, 0), 1, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 0), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 0), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 0), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 1), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 1), 1, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 1), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 1), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 2), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 2), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 2), 1, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 2), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 3), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 3), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 3), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 3), 1, 0);
        }

        // SetTranslationMatrix
        {
            let m = new ez.Mat4();
            m.SetTranslationMatrix(new ez.Vec3(2, 3, 4));

            EZ_TEST.FLOAT(m.GetElement(0, 0), 1, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 0), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 0), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 0), 2, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 1), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 1), 1, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 1), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 1), 3, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 2), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 2), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 2), 1, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 2), 4, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 3), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 3), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 3), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 3), 1, 0);
        }

        // SetScalingMatrix
        {
            let m = new ez.Mat4();
            m.SetScalingMatrix(new ez.Vec3(2, 3, 4));

            EZ_TEST.FLOAT(m.GetElement(0, 0), 2, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 0), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 0), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 0), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 1), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 1), 3, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 1), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 1), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 2), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 2), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 2), 4, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 2), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 3), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 3), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 3), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 3), 1, 0);
        }

        // SetRotationMatrixX
        {
            let m = new ez.Mat4();

            m.SetRotationMatrixX(ez.Angle.DegreeToRadian(90));

            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(1, -3, 2), 0.0001));

            m.SetRotationMatrixX(ez.Angle.DegreeToRadian(180));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(1, -2, -3), 0.0001));

            m.SetRotationMatrixX(ez.Angle.DegreeToRadian(270));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(1, 3, -2), 0.0001));

            m.SetRotationMatrixX(ez.Angle.DegreeToRadian(360));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(1, 2, 3), 0.0001));
        }

        // SetRotationMatrixY
        {
            let m = new ez.Mat4();

            m.SetRotationMatrixY(ez.Angle.DegreeToRadian(90));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(3, 2, -1), 0.0001));

            m.SetRotationMatrixY(ez.Angle.DegreeToRadian(180));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(-1, 2, -3), 0.0001));

            m.SetRotationMatrixY(ez.Angle.DegreeToRadian(270));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(-3, 2, 1), 0.0001));

            m.SetRotationMatrixY(ez.Angle.DegreeToRadian(360));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(1, 2, 3), 0.0001));
        }

        // SetRotationMatrixZ
        {
            let m = new ez.Mat4();

            m.SetRotationMatrixZ(ez.Angle.DegreeToRadian(90));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(-2, 1, 3), 0.0001));

            m.SetRotationMatrixZ(ez.Angle.DegreeToRadian(180));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(-1, -2, 3), 0.0001));

            m.SetRotationMatrixZ(ez.Angle.DegreeToRadian(270));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(2, -1, 3), 0.0001));

            m.SetRotationMatrixZ(ez.Angle.DegreeToRadian(360));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(1, 2, 3), 0.0001));
        }

        // SetRotationMatrix
        {
            let m = new ez.Mat4();

            m.SetRotationMatrix(new ez.Vec3(1, 0, 0), ez.Angle.DegreeToRadian(90));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(1, -3, 2), 0.001));

            m.SetRotationMatrix(new ez.Vec3(1, 0, 0), ez.Angle.DegreeToRadian(180));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(1, -2, -3), 0.001));

            m.SetRotationMatrix(new ez.Vec3(1, 0, 0), ez.Angle.DegreeToRadian(270));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(1, 3, -2), 0.001));

            m.SetRotationMatrix(new ez.Vec3(0, 1, 0), ez.Angle.DegreeToRadian(90));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(3, 2, -1), 0.001));

            m.SetRotationMatrix(new ez.Vec3(0, 1, 0), ez.Angle.DegreeToRadian(180));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(-1, 2, -3), 0.001));

            m.SetRotationMatrix(new ez.Vec3(0, 1, 0), ez.Angle.DegreeToRadian(270));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(-3, 2, 1), 0.001));

            m.SetRotationMatrix(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(90));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(-2, 1, 3), 0.001));

            m.SetRotationMatrix(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(180));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(-1, -2, 3), 0.001));

            m.SetRotationMatrix(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(270));
            EZ_TEST.BOOL(mul(m, new ez.Vec3(1, 2, 3)).IsEqual(new ez.Vec3(2, -1, 3), 0.001));
        }

        // IdentityMatrix
        {
            let m = ez.Mat4.IdentityMatrix();

            EZ_TEST.FLOAT(m.GetElement(0, 0), 1, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 0), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 0), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 0), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 1), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 1), 1, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 1), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 1), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 2), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 2), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 2), 1, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 2), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 3), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 3), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 3), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 3), 1, 0);
        }

        // ZeroMatrix
        {
            let m = ez.Mat4.ZeroMatrix();

            EZ_TEST.FLOAT(m.GetElement(0, 0), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 0), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 0), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 0), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 1), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 1), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 1), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 1), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 2), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 2), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 2), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 2), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 3), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 3), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 3), 0, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 3), 0, 0);
        }

        // Transpose
        {
            let m = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            m.Transpose();

            EZ_TEST.FLOAT(m.GetElement(0, 0), 1, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 0), 5, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 0), 9, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 0), 13, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 1), 2, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 1), 6, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 1), 10, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 1), 14, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 2), 3, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 2), 7, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 2), 11, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 2), 15, 0);
            EZ_TEST.FLOAT(m.GetElement(0, 3), 4, 0);
            EZ_TEST.FLOAT(m.GetElement(1, 3), 8, 0);
            EZ_TEST.FLOAT(m.GetElement(2, 3), 12, 0);
            EZ_TEST.FLOAT(m.GetElement(3, 3), 16, 0);
        }

        // GetTranspose
        {
            let m0 = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            let m = m0.GetTranspose();

            EZ_TEST.FLOAT(m.GetElement(0, 0), 1);
            EZ_TEST.FLOAT(m.GetElement(1, 0), 5);
            EZ_TEST.FLOAT(m.GetElement(2, 0), 9);
            EZ_TEST.FLOAT(m.GetElement(3, 0), 13);
            EZ_TEST.FLOAT(m.GetElement(0, 1), 2);
            EZ_TEST.FLOAT(m.GetElement(1, 1), 6);
            EZ_TEST.FLOAT(m.GetElement(2, 1), 10);
            EZ_TEST.FLOAT(m.GetElement(3, 1), 14);
            EZ_TEST.FLOAT(m.GetElement(0, 2), 3);
            EZ_TEST.FLOAT(m.GetElement(1, 2), 7);
            EZ_TEST.FLOAT(m.GetElement(2, 2), 11);
            EZ_TEST.FLOAT(m.GetElement(3, 2), 15);
            EZ_TEST.FLOAT(m.GetElement(0, 3), 4);
            EZ_TEST.FLOAT(m.GetElement(1, 3), 8);
            EZ_TEST.FLOAT(m.GetElement(2, 3), 12);
            EZ_TEST.FLOAT(m.GetElement(3, 3), 16);
        }

        // Invert
        {
            for (let x = 1.0; x < 360.0; x += 40.0) {
                for (let y = 2.0; y < 360.0; y += 37.0) {
                    for (let z = 3.0; z < 360.0; z += 53.0) {
                        let m = new ez.Mat4();
                        let inv = new ez.Mat4();

                        m.SetRotationMatrix(new ez.Vec3(x, y, z).GetNormalized(), ez.Angle.DegreeToRadian(19.0));
                        inv = m.Clone();
                        EZ_TEST.BOOL(inv.Invert());

                        let v = mul(m, new ez.Vec3(1, 1, 1));
                        let vinv = mul(inv, v);

                        EZ_TEST.VEC3(vinv, new ez.Vec3(1, 1, 1), 0.001);
                    }
                }
            }
        }

        // GetInverse
        {
            for (let x = 1.0; x < 360.0; x += 39.0) {
                for (let y = 2.0; y < 360.0; y += 29.0) {
                    for (let z = 3.0; z < 360.0; z += 51.0) {
                        let m = new ez.Mat4();
                        let inv = new ez.Mat4();

                        m.SetRotationMatrix(new ez.Vec3(x, y, z).GetNormalized(), ez.Angle.DegreeToRadian(83.0));
                        inv = m.GetInverse();

                        let v = mul(m, new ez.Vec3(1, 1, 1));
                        let vinv = mul(inv, v);

                        EZ_TEST.VEC3(vinv, new ez.Vec3(1, 1, 1), 0.001);
                    }
                }
            }
        }

        // IsZero
        {
            let m = new ez.Mat4();

            m.SetIdentity();
            EZ_TEST.BOOL(!m.IsZero());

            m.SetZero();
            EZ_TEST.BOOL(m.IsZero());
        }

        // IsIdentity
        {
            let m = new ez.Mat4();

            m.SetIdentity();
            EZ_TEST.BOOL(m.IsIdentity());

            m.SetZero();
            EZ_TEST.BOOL(!m.IsIdentity());
        }

        // GetRow
        {
            let m = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            EZ_TEST.ARRAY(4, m.GetRow(0), [1, 2, 3, 4], 0.0);
            EZ_TEST.ARRAY(4, m.GetRow(1), [5, 6, 7, 8], 0.0);
            EZ_TEST.ARRAY(4, m.GetRow(2), [9, 10, 11, 12], 0.0);
            EZ_TEST.ARRAY(4, m.GetRow(3), [13, 14, 15, 16], 0.0);
        }

        // SetRow
        {
            let m = new ez.Mat4();
            m.SetZero();

            m.SetRow(0, 1, 2, 3, 4);
            EZ_TEST.ARRAY(4, m.GetRow(0), [1, 2, 3, 4], 0.0);

            m.SetRow(1, 5, 6, 7, 8);
            EZ_TEST.ARRAY(4, m.GetRow(1), [5, 6, 7, 8], 0.0);

            m.SetRow(2, 9, 10, 11, 12);
            EZ_TEST.ARRAY(4, m.GetRow(2), [9, 10, 11, 12], 0.0);

            m.SetRow(3, 13, 14, 15, 16);
            EZ_TEST.ARRAY(4, m.GetRow(3), [13, 14, 15, 16], 0.0);
        }

        // GetColumn
        {
            let m = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            EZ_TEST.ARRAY(4, m.GetColumn(0), [1, 5, 9, 13], 0.0);
            EZ_TEST.ARRAY(4, m.GetColumn(1), [2, 6, 10, 14], 0.0);
            EZ_TEST.ARRAY(4, m.GetColumn(2), [3, 7, 11, 15], 0.0);
            EZ_TEST.ARRAY(4, m.GetColumn(3), [4, 8, 12, 16], 0.0);
        }

        // SetColumn
        {
            let m = new ez.Mat4();
            m.SetZero();

            m.SetColumn(0, 1, 2, 3, 4);
            EZ_TEST.ARRAY(4, m.GetColumn(0), [1, 2, 3, 4], 0.0);

            m.SetColumn(1, 5, 6, 7, 8);
            EZ_TEST.ARRAY(4, m.GetColumn(1), [5, 6, 7, 8], 0.0);

            m.SetColumn(2, 9, 10, 11, 12);
            EZ_TEST.ARRAY(4, m.GetColumn(2), [9, 10, 11, 12], 0.0);

            m.SetColumn(3, 13, 14, 15, 16);
            EZ_TEST.ARRAY(4, m.GetColumn(3), [13, 14, 15, 16], 0.0);
        }

        // GetDiagonal
        {
            let m = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            EZ_TEST.ARRAY(4, m.GetDiagonal(), [1, 6, 11, 16], 0.0);
        }

        // SetDiagonal
        {
            let m = new ez.Mat4();
            m.SetZero();

            m.SetDiagonal(1, 2, 3, 4);
            EZ_TEST.ARRAY(4, m.GetColumn(0), [1, 0, 0, 0], 0.0);
            EZ_TEST.ARRAY(4, m.GetColumn(1), [0, 2, 0, 0], 0.0);
            EZ_TEST.ARRAY(4, m.GetColumn(2), [0, 0, 3, 0], 0.0);
            EZ_TEST.ARRAY(4, m.GetColumn(3), [0, 0, 0, 4], 0.0);
        }

        // GetTranslationVector
        {
            let m = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            EZ_TEST.VEC3(m.GetTranslationVector(), new ez.Vec3(4, 8, 12), 0.0);
        }

        // SetTranslationVector
        {
            let m = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            m.SetTranslationVector(new ez.Vec3(17, 18, 19));
            EZ_TEST.ARRAY(4, m.GetRow(0), [1, 2, 3, 17], 0.0);
            EZ_TEST.ARRAY(4, m.GetRow(1), [5, 6, 7, 18], 0.0);
            EZ_TEST.ARRAY(4, m.GetRow(2), [9, 10, 11, 19], 0.0);
            EZ_TEST.ARRAY(4, m.GetRow(3), [13, 14, 15, 16], 0.0);
        }

        // SetRotationalPart
        {
            let m = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            let r = new ez.Mat3(17, 18, 19, 20, 21, 22, 23, 24, 25);

            m.SetRotationalPart(r);
            EZ_TEST.ARRAY(4, m.GetRow(0), [17, 18, 19, 4], 0.0);
            EZ_TEST.ARRAY(4, m.GetRow(1), [20, 21, 22, 8], 0.0);
            EZ_TEST.ARRAY(4, m.GetRow(2), [23, 24, 25, 12], 0.0);
            EZ_TEST.ARRAY(4, m.GetRow(3), [13, 14, 15, 16], 0.0);
        }

        // GetRotationalPart
        {
            let m = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            let r = m.GetRotationalPart();
            EZ_TEST.ARRAY(3, r.GetRow(0), [1, 2, 3], 0.0);
            EZ_TEST.ARRAY(3, r.GetRow(1), [5, 6, 7], 0.0);
            EZ_TEST.ARRAY(3, r.GetRow(2), [9, 10, 11], 0.0);
        }

        // GetScalingFactors
        {
            let m = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            let s = m.GetScalingFactors();
            EZ_TEST.VEC3(s,
                new ez.Vec3(Math.sqrt((1 * 1 + 5 * 5 + 9 * 9)), Math.sqrt((2 * 2 + 6 * 6 + 10 * 10)),
                    Math.sqrt((3 * 3 + 7 * 7 + 11 * 11))),
                0.0001);
        }

        // SetScalingFactors
        {
            let m = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            EZ_TEST.BOOL(m.SetScalingFactors(1, 2, 3));

            let s = m.GetScalingFactors();
            EZ_TEST.VEC3(s, new ez.Vec3(1, 2, 3), 0.0001);
        }

        // TransformDirection
        {
            let m = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            let r = new ez.Vec3(1, 2, 3);
            m.TransformDirection(r);

            EZ_TEST.VEC3(r, new ez.Vec3(1 * 1 + 2 * 2 + 3 * 3, 1 * 5 + 2 * 6 + 3 * 7, 1 * 9 + 2 * 10 + 3 * 11), 0.0001);
        }

        // TransformPosition
        {
            let m = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            let r = new ez.Vec3(1, 2, 3);
            m.TransformPosition(r);

            EZ_TEST.VEC3(r, new ez.Vec3(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), 0.0001);
        }

        // IsIdentical
        {
            let m = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            let m2 = m.Clone();

            EZ_TEST.BOOL(m.IsIdentical(m2));

            m2.m_ElementsCM[0] += 0.001;
            EZ_TEST.BOOL(!m.IsIdentical(m2));
        }

        // IsEqual
        {
            let m = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            let m2 = m.Clone();

            EZ_TEST.BOOL(m.IsEqual(m2, 0.0001));

            m2.m_ElementsCM[0] += 0.001;
            EZ_TEST.BOOL(m.IsEqual(m2, 0.001));
            EZ_TEST.BOOL(!m.IsEqual(m2, 0.0001));
        }

        // SetMulMat4
        {
            let m1 = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            let m2 = new ez.Mat4(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16);

            let r = new ez.Mat4();
            r.SetMulMat4(m1, m2);

            EZ_TEST.ARRAY(4, r.GetColumn(0),
                [-1 * 1 + -5 * 2 + -9 * 3 + -13 * 4, -1 * 5 + -5 * 6 + -9 * 7 + -13 * 8, -1 * 9 + -5 * 10 + -9 * 11 + -13 * 12,
                -1 * 13 + -5 * 14 + -9 * 15 + -13 * 16],
                0.001);
            EZ_TEST.ARRAY(4, r.GetColumn(1),
                [-2 * 1 + -6 * 2 + -10 * 3 + -14 * 4, -2 * 5 + -6 * 6 + -10 * 7 + -14 * 8, -2 * 9 + -6 * 10 + -10 * 11 + -14 * 12,
                -2 * 13 + -6 * 14 + -10 * 15 + -14 * 16],
                0.001);
            EZ_TEST.ARRAY(4, r.GetColumn(2),
                [-3 * 1 + -7 * 2 + -11 * 3 + -15 * 4, -3 * 5 + -7 * 6 + -11 * 7 + -15 * 8, -3 * 9 + -7 * 10 + -11 * 11 + -15 * 12,
                -3 * 13 + -7 * 14 + -11 * 15 + -15 * 16],
                0.001);
            EZ_TEST.ARRAY(4, r.GetColumn(3),
                [-4 * 1 + -8 * 2 + -12 * 3 + -16 * 4, -4 * 5 + -8 * 6 + -12 * 7 + -16 * 8, -4 * 9 + -8 * 10 + -12 * 11 + -16 * 12,
                -4 * 13 + -8 * 14 + -12 * 15 + -16 * 16],
                0.001);
        }

        // MulNumber
        {
            let m0 = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            let m = m0.Clone();
            m.MulNumber(2);

            EZ_TEST.ARRAY(4, m.GetRow(0), [2, 4, 6, 8], 0.0001);
            EZ_TEST.ARRAY(4, m.GetRow(1), [10, 12, 14, 16], 0.0001);
            EZ_TEST.ARRAY(4, m.GetRow(2), [18, 20, 22, 24], 0.0001);
            EZ_TEST.ARRAY(4, m.GetRow(3), [26, 28, 30, 32], 0.0001);
        }

        // DivNumber
        {
            let m0 = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            m0.MulNumber(4);

            let m = m0.Clone();
            m.DivNumber(2);

            EZ_TEST.ARRAY(4, m.GetRow(0), [2, 4, 6, 8], 0.0001);
            EZ_TEST.ARRAY(4, m.GetRow(1), [10, 12, 14, 16], 0.0001);
            EZ_TEST.ARRAY(4, m.GetRow(2), [18, 20, 22, 24], 0.0001);
            EZ_TEST.ARRAY(4, m.GetRow(3), [26, 28, 30, 32], 0.0001);
        }

        // AddMat4 / SubMat4
        {
            let m0 = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            let m1 = new ez.Mat4(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16);

            let r1 = m0.Clone();
            r1.AddMat4(m1);

            let r2 = m0.Clone();
            r2.SubMat4(m1);

            let c2 = m0.Clone();
            c2.MulNumber(2);

            EZ_TEST.BOOL(r1.IsZero());
            EZ_TEST.BOOL(r2.IsEqual(c2, 0.0001));
        }

        // IsIdentical
        {
            let m = new ez.Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

            let m2 = m.Clone();

            EZ_TEST.BOOL(m.IsIdentical(m2));

            m2.m_ElementsCM[0] += 0.001;

            EZ_TEST.BOOL(!m.IsIdentical(m2));
        }

    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "TestMat4") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

