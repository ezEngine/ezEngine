import ez = require("TypeScript/ez")
import EZ_TEST = require("./TestFramework")

export class TestQuat extends ez.TypescriptComponent {

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
        {
            let q = new ez.Quat();
            EZ_TEST.FLOAT(q.x, 0, 0.001);
            EZ_TEST.FLOAT(q.y, 0, 0.001);
            EZ_TEST.FLOAT(q.z, 0, 0.001);
            EZ_TEST.FLOAT(q.w, 1, 0.001);
        }

        // Clone / Normalize
        {
            let q = new ez.Quat(1, 2, 3, 4);
            EZ_TEST.BOOL(q.IsIdentical(new ez.Quat(1, 2, 3, 4)));

            q.Normalize();
            EZ_TEST.QUAT(q.Clone(), q, 0.001);
        }

        // SetIdentity
        {
            let q = new ez.Quat(1, 2, 3, 4);
            q.SetIdentity();
            EZ_TEST.QUAT(q, ez.Quat.IdentityQuaternion(), 0.001);
        }


        // IdentityQuaternion
        {
            let q = ez.Quat.IdentityQuaternion();

            EZ_TEST.FLOAT(q.x, 0, 0.001);
            EZ_TEST.FLOAT(q.y, 0, 0.001);
            EZ_TEST.FLOAT(q.z, 0, 0.001);
            EZ_TEST.FLOAT(q.w, 1, 0.001);
        }

        // SetFromAxisAndAngle / RotateVec3
        {
            {
                let q = new ez.Quat();
                q.SetFromAxisAndAngle(new ez.Vec3(1, 0, 0), ez.Angle.DegreeToRadian(90));

                let v = new ez.Vec3(0, 1, 0);
                q.RotateVec3(v);
                EZ_TEST.VEC3(v, new ez.Vec3(0, 0, 1), 0.0001);

                let v2 = new ez.Vec3(0, 1, 0);
                q.InvRotateVec3(v2);
                EZ_TEST.VEC3(v2, new ez.Vec3(0, 0, -1), 0.0001);
            }

            {
                let q = new ez.Quat();
                q.SetFromAxisAndAngle(new ez.Vec3(0, 1, 0), ez.Angle.DegreeToRadian(90));

                let v = new ez.Vec3(1, 0, 0);
                q.RotateVec3(v);
                EZ_TEST.VEC3(v, new ez.Vec3(0, 0, -1), 0.0001);

                let v2 = new ez.Vec3(1, 0, 0);
                q.InvRotateVec3(v2);
                EZ_TEST.VEC3(v2, new ez.Vec3(0, 0, 1), 0.0001);
            }

            {
                let q = new ez.Quat();
                q.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(90));

                let v = new ez.Vec3(0, 1, 0);
                q.RotateVec3(v);
                EZ_TEST.VEC3(v, new ez.Vec3(-1, 0, 0), 0.0001);

                let v2 = new ez.Vec3(0, 1, 0);
                q.InvRotateVec3(v2);
                EZ_TEST.VEC3(v2, new ez.Vec3(1, 0, 0), 0.0001);
            }
        }

        // SetQuat
        {
            let q = new ez.Quat(1, 2, 3, 4);
            let q2 = new ez.Quat();
            q2.SetQuat(q);

            EZ_TEST.FLOAT(q2.x, 1, 0.001);
            EZ_TEST.FLOAT(q2.y, 2, 0.001);
            EZ_TEST.FLOAT(q2.z, 3, 0.001);
            EZ_TEST.FLOAT(q2.w, 4, 0.001);
        }

        // SetFromMat3
        {
            let m = new ez.Mat3();
            m.SetRotationMatrixZ(ez.Angle.DegreeToRadian(-90));

            let q1 = new ez.Quat();
            let q2 = new ez.Quat();
            let q3 = new ez.Quat();

            q1.SetFromMat3(m);
            q2.SetFromAxisAndAngle(new ez.Vec3(0, 0, -1), ez.Angle.DegreeToRadian(90));
            q3.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(-90));

            EZ_TEST.BOOL(q1.IsEqualRotation(q2, 0.001));
            EZ_TEST.BOOL(q1.IsEqualRotation(q3, 0.001));
        }

        // SetSlerp
        {
            let q1 = new ez.Quat();
            let q2 = new ez.Quat();
            let q3 = new ez.Quat();
            let qr = new ez.Quat();

            q1.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(45));
            q2.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(0));
            q3.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(90));

            qr.SetSlerp(q2, q3, 0.5);

            EZ_TEST.QUAT(q1, qr, 0.0001);
        }

        // GetRotationAxisAndAngle
        {
            let q1 = new ez.Quat();
            let q2 = new ez.Quat();
            let q3 = new ez.Quat();

            q1.SetShortestRotation(new ez.Vec3(0, 1, 0), new ez.Vec3(1, 0, 0));
            q2.SetFromAxisAndAngle(new ez.Vec3(0, 0, -1), ez.Angle.DegreeToRadian(90));
            q3.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(-90));

            let res = q1.GetRotationAxisAndAngle();
            EZ_TEST.VEC3(res.axis, new ez.Vec3(0, 0, -1), 0.001);
            EZ_TEST.FLOAT(ez.Angle.RadianToDegree(res.angleInRadian), 90, 0.001);

            res = q2.GetRotationAxisAndAngle();
            EZ_TEST.VEC3(res.axis, new ez.Vec3(0, 0, -1), 0.001);
            EZ_TEST.FLOAT(ez.Angle.RadianToDegree(res.angleInRadian), 90, 0.001);

            res = q3.GetRotationAxisAndAngle();
            EZ_TEST.VEC3(res.axis, new ez.Vec3(0, 0, -1), 0.001);
            EZ_TEST.FLOAT(ez.Angle.RadianToDegree(res.angleInRadian), 90, 0.001);

            res = ez.Quat.IdentityQuaternion().GetRotationAxisAndAngle();
            EZ_TEST.VEC3(res.axis, new ez.Vec3(1, 0, 0), 0.001);
            EZ_TEST.FLOAT(ez.Angle.RadianToDegree(res.angleInRadian), 0, 0.001);

            let otherIdentity = new ez.Quat(0, 0, 0, -1);
            res = otherIdentity.GetRotationAxisAndAngle();
            EZ_TEST.VEC3(res.axis, new ez.Vec3(1, 0, 0), 0.001);
            EZ_TEST.FLOAT(ez.Angle.RadianToDegree(res.angleInRadian), 360, 0.001);
        }

        // GetAsMat3
        {
            let q = new ez.Quat();
            q.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(90));

            let mr = new ez.Mat3();
            mr.SetRotationMatrixZ(ez.Angle.DegreeToRadian(90));

            let m = q.GetAsMat3();

            EZ_TEST.BOOL(mr.IsEqual(m, 0.001));
        }

        // GetAsMat4
        {
            let q = new ez.Quat();
            q.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(90));

            let mr = new ez.Mat4();
            mr.SetRotationMatrixZ(ez.Angle.DegreeToRadian(90));

            let m = q.GetAsMat4();

            EZ_TEST.BOOL(mr.IsEqual(m, 0.001));
        }

        // SetShortestRotation / IsEqualRotation
        {
            let q1 = new ez.Quat();
            let q2 = new ez.Quat();
            let q3 = new ez.Quat();

            q1.SetShortestRotation(new ez.Vec3(0, 1, 0), new ez.Vec3(1, 0, 0));
            q2.SetFromAxisAndAngle(new ez.Vec3(0, 0, -1), ez.Angle.DegreeToRadian(90));
            q3.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(-90));

            EZ_TEST.BOOL(q1.IsEqualRotation(q2, 0.001));
            EZ_TEST.BOOL(q1.IsEqualRotation(q3, 0.001));

            EZ_TEST.BOOL(ez.Quat.IdentityQuaternion().IsEqualRotation(ez.Quat.IdentityQuaternion(), 0.001));
            EZ_TEST.BOOL(ez.Quat.IdentityQuaternion().IsEqualRotation(new ez.Quat(0, 0, 0, -1), 0.001));
        }

        // SetConcatenatedRotations / ConcatenateRotations
        {
            let q1 = new ez.Quat();
            let q2 = new ez.Quat();
            let q3 = new ez.Quat();
            let qr = new ez.Quat();
            let qr2 = new ez.Quat();

            q1.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(60));
            q2.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(30));
            q3.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(90));

            qr.SetConcatenatedRotations(q1, q2);

            EZ_TEST.BOOL(qr.IsEqualRotation(q3, 0.0001));

            qr2 = q1.Clone();
            qr2.ConcatenateRotations(q2);

            EZ_TEST.QUAT(qr, qr2, 0.001);
        }

        // IsIdentical
        {
            let q1 = new ez.Quat();
            let q2 = new ez.Quat();

            q1.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(60));
            q2.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(30));
            EZ_TEST.BOOL(!q1.IsIdentical(q2));

            q2.SetFromAxisAndAngle(new ez.Vec3(1, 0, 0), ez.Angle.DegreeToRadian(60));
            EZ_TEST.BOOL(!q1.IsIdentical(q2));

            q2.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(60));
            EZ_TEST.BOOL(q1.IsIdentical(q2));
        }

        // Negate / GetNegated
        {
            let q = new ez.Quat();
            q.SetFromAxisAndAngle(new ez.Vec3(1, 0, 0), ez.Angle.DegreeToRadian(90));

            let v = new ez.Vec3(0, 1, 0);
            q.RotateVec3(v);
            EZ_TEST.VEC3(v, new ez.Vec3(0, 0, 1), 0.0001);

            let n1 = q.GetNegated();
            let n2 = q.Clone();
            n2.Negate();

            EZ_TEST.QUAT(n1, n2, 0.001);

            let v2 = new ez.Vec3(0, 1, 0);
            n1.RotateVec3(v2);
            EZ_TEST.VEC3(v2, new ez.Vec3(0, 0, -1), 0.0001);
        }

        // SetFromEulerAngles / GetAsEulerAngles
        {
            let q = new ez.Quat();
            q.SetFromEulerAngles(ez.Angle.DegreeToRadian(90), 0, 0);

            let euler = q.GetAsEulerAngles();
            EZ_TEST.FLOAT(euler.roll, ez.Angle.DegreeToRadian(90), 0.001);
            EZ_TEST.FLOAT(euler.pitch, ez.Angle.DegreeToRadian(0), 0.001);
            EZ_TEST.FLOAT(euler.yaw, ez.Angle.DegreeToRadian(0), 0.001);

            q.SetFromEulerAngles(0, ez.Angle.DegreeToRadian(90), 0);
            euler = q.GetAsEulerAngles();
            EZ_TEST.FLOAT(euler.pitch, ez.Angle.DegreeToRadian(90), 0.001);

            // due to compilation differences, this the result for this computation can be very different (but equivalent)
            EZ_TEST.BOOL((ez.Utils.IsNumberEqual(euler.roll, ez.Angle.DegreeToRadian(180), 0.001) && 
                          ez.Utils.IsNumberEqual(euler.yaw, ez.Angle.DegreeToRadian(180), 0.001)) ||
                          (ez.Utils.IsNumberEqual(euler.roll, ez.Angle.DegreeToRadian(0), 0.001) && 
                          ez.Utils.IsNumberEqual(euler.yaw, ez.Angle.DegreeToRadian(0), 0.001)));


            q.SetFromEulerAngles(0, 0, ez.Angle.DegreeToRadian(90));
            euler = q.GetAsEulerAngles();
            EZ_TEST.FLOAT(euler.roll, ez.Angle.DegreeToRadian(0), 0.001);
            EZ_TEST.FLOAT(euler.pitch, ez.Angle.DegreeToRadian(0), 0.001);
            EZ_TEST.FLOAT(euler.yaw, ez.Angle.DegreeToRadian(90), 0.001);
        }
    }


    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "TestQuat") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

