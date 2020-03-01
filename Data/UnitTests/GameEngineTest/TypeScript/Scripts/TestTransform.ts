import ez = require("TypeScript/ez")
import EZ_TEST = require("./TestFramework")

export class TestTransform extends ez.TypescriptComponent {

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
            let t = new ez.Transform();

            EZ_TEST.VEC3(t.position, ez.Vec3.ZeroVector());
            EZ_TEST.QUAT(t.rotation, ez.Quat.IdentityQuaternion());
            EZ_TEST.VEC3(t.scale, ez.Vec3.OneVector());
        }

        // Clone
        {
            let t = new ez.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(ez.Vec3.UnitAxisZ(), ez.Angle.DegreeToRadian(90));
            t.scale.Set(4, 5, 6);

            let c = t.Clone();
            EZ_TEST.BOOL(t != c);
            EZ_TEST.BOOL(t.position != c.position);
            EZ_TEST.BOOL(t.rotation != c.rotation);
            EZ_TEST.BOOL(t.scale != c.scale);

            EZ_TEST.VEC3(t.position, c.position);
            EZ_TEST.QUAT(t.rotation, c.rotation);
            EZ_TEST.VEC3(t.scale, c.scale);
        }

        // SetTransform
        {
            let t = new ez.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(ez.Vec3.UnitAxisZ(), ez.Angle.DegreeToRadian(90));
            t.scale.Set(4, 5, 6);

            let c = new ez.Transform();
            c.SetTransform(t);

            EZ_TEST.BOOL(t != c);
            EZ_TEST.BOOL(t.position != c.position);
            EZ_TEST.BOOL(t.rotation != c.rotation);
            EZ_TEST.BOOL(t.scale != c.scale);

            EZ_TEST.VEC3(t.position, c.position);
            EZ_TEST.QUAT(t.rotation, c.rotation);
            EZ_TEST.VEC3(t.scale, c.scale);
        }

        // SetIdentity
        {
            let t = new ez.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(ez.Vec3.UnitAxisZ(), ez.Angle.DegreeToRadian(90));
            t.scale.Set(4, 5, 6);

            t.SetIdentity();

            EZ_TEST.VEC3(t.position, ez.Vec3.ZeroVector());
            EZ_TEST.QUAT(t.rotation, ez.Quat.IdentityQuaternion());
            EZ_TEST.VEC3(t.scale, ez.Vec3.OneVector());
        }

        // IdentityTransform
        {
            let t = ez.Transform.IdentityTransform();

            EZ_TEST.VEC3(t.position, ez.Vec3.ZeroVector());
            EZ_TEST.QUAT(t.rotation, ez.Quat.IdentityQuaternion());
            EZ_TEST.VEC3(t.scale, ez.Vec3.OneVector());
        }

        // IsIdentical
        {
            let t = new ez.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(ez.Vec3.UnitAxisZ(), ez.Angle.DegreeToRadian(90));
            t.scale.Set(4, 5, 6);

            let c = new ez.Transform();
            c.SetTransform(t);

            EZ_TEST.BOOL(t.IsIdentical(c));

            c.position.x += 0.0001;

            EZ_TEST.BOOL(!t.IsIdentical(c));
        }

        // IsEqual
        {
            let t = new ez.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(ez.Vec3.UnitAxisZ(), ez.Angle.DegreeToRadian(90));
            t.scale.Set(4, 5, 6);

            let c = new ez.Transform();
            c.SetTransform(t);

            EZ_TEST.BOOL(t.IsEqual(c));

            c.position.x += 0.0001;

            EZ_TEST.BOOL(t.IsEqual(c, 0.001));
            EZ_TEST.BOOL(!t.IsEqual(c, 0.00001));
        }

        // Translate
        {
            let t = new ez.Transform();
            t.Translate(new ez.Vec3(1, 2, 3));

            EZ_TEST.VEC3(t.position, new ez.Vec3(1, 2, 3));
            EZ_TEST.QUAT(t.rotation, ez.Quat.IdentityQuaternion());
            EZ_TEST.VEC3(t.scale, ez.Vec3.OneVector());
        }

        // SetMulTransform / MulTransform
        {
            let tParent = new ez.Transform();
            tParent.position.Set(1, 2, 3);

            tParent.rotation.SetFromAxisAndAngle(new ez.Vec3(0, 1, 0), ez.Angle.DegreeToRadian(90));
            tParent.scale.SetAll(2);

            let tToChild = new ez.Transform();
            tToChild.position.Set(4, 5, 6);

            tToChild.rotation.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(90));
            tToChild.scale.SetAll(4);

            // this is exactly the same as SetGlobalTransform
            let tChild = new ez.Transform();
            tChild.SetMulTransform(tParent, tToChild);

            EZ_TEST.BOOL(tChild.position.IsEqual(new ez.Vec3(13, 12, -5), 0.0001));

            let q1 = new ez.Quat();
            q1.SetConcatenatedRotations(tParent.rotation, tToChild.rotation);
            EZ_TEST.BOOL(tChild.rotation.IsEqualRotation(q1, 0.0001));

            EZ_TEST.VEC3(tChild.scale, new ez.Vec3(8, 8, 8));

            tChild = tParent.Clone();
            tChild.MulTransform(tToChild);

            EZ_TEST.BOOL(tChild.position.IsEqual(new ez.Vec3(13, 12, -5), 0.0001));

            q1.SetConcatenatedRotations(tParent.rotation, tToChild.rotation);
            EZ_TEST.QUAT(tChild.rotation, q1);
            EZ_TEST.VEC3(tChild.scale, new ez.Vec3(8, 8, 8));

            let a = new ez.Vec3(7, 8, 9);
            let b = a.Clone();
            tToChild.TransformPosition(b);
            tParent.TransformPosition(b);

            let c = a.Clone();
            tChild.TransformPosition(c);

            EZ_TEST.VEC3(b, c);
        }

        // Invert / GetInverse
        {
            let tParent = new ez.Transform();
            tParent.position.Set(1, 2, 3);

            tParent.rotation.SetFromAxisAndAngle(new ez.Vec3(0, 1, 0), ez.Angle.DegreeToRadian(90));
            tParent.scale.SetAll(2);

            let tToChild = new ez.Transform();
            tParent.position.Set(4, 5, 6);

            tToChild.rotation.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(90));
            tToChild.scale.SetAll(4);

            let tChild = new ez.Transform();
            tChild.SetMulTransform(tParent, tToChild);

            // invert twice -> get back original
            let t2 = tToChild.Clone();
            t2.Invert();
            EZ_TEST.BOOL(!t2.IsEqual(tToChild, 0.0001));
            t2 = t2.GetInverse();
            EZ_TEST.BOOL(t2.IsEqual(tToChild, 0.0001));

            let tInvToChild = tToChild.GetInverse();

            let tParentFromChild = new ez.Transform();
            tParentFromChild.SetMulTransform(tChild, tInvToChild);

            EZ_TEST.BOOL(tParent.IsEqual(tParentFromChild, 0.0001));
        }

        // SetLocalTransform
        {
            let q = new ez.Quat();
            q.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(90));

            let tParent = new ez.Transform();
            tParent.position.Set(1, 2, 3);
            tParent.rotation.SetFromAxisAndAngle(new ez.Vec3(0, 1, 0), ez.Angle.DegreeToRadian(90));
            tParent.scale.SetAll(2);

            let tChild = new ez.Transform();
            tChild.position.Set(13, 12, -5);
            tChild.rotation.SetConcatenatedRotations(tParent.rotation, q);
            tChild.scale.SetAll(8);

            let tToChild = new ez.Transform();
            tToChild.SetLocalTransform(tParent, tChild);

            EZ_TEST.VEC3(tToChild.position, new ez.Vec3(4, 5, 6));
            EZ_TEST.QUAT(tToChild.rotation, q);
            EZ_TEST.VEC3(tToChild.scale, new ez.Vec3(4, 4, 4));
        }

        // SetGlobalTransform
        {
            let tParent = new ez.Transform();
            tParent.position.Set(1, 2, 3);
            tParent.rotation.SetFromAxisAndAngle(new ez.Vec3(0, 1, 0), ez.Angle.DegreeToRadian(90));
            tParent.scale.SetAll(2);

            let tToChild = new ez.Transform();
            tToChild.position.Set(4, 5, 6);
            tToChild.rotation.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(90));
            tToChild.scale.SetAll(4);

            let tChild = new ez.Transform();
            tChild.SetGlobalTransform(tParent, tToChild);

            EZ_TEST.VEC3(tChild.position, new ez.Vec3(13, 12, -5));

            let q = new ez.Quat();
            q.SetConcatenatedRotations(tParent.rotation, tToChild.rotation);
            EZ_TEST.QUAT(tChild.rotation, q);
            EZ_TEST.VEC3(tChild.scale, new ez.Vec3(8, 8, 8));
        }

        // TransformPosition / TransformDirection
        {
            let qRotX = new ez.Quat();
            let qRotY = new ez.Quat();

            qRotX.SetFromAxisAndAngle(new ez.Vec3(1, 0, 0), ez.Angle.DegreeToRadian(90));
            qRotY.SetFromAxisAndAngle(new ez.Vec3(0, 1, 0), ez.Angle.DegreeToRadian(90));

            let t = new ez.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetConcatenatedRotations(qRotY, qRotX);
            t.scale.Set(2, -2, 4);

            let v = new ez.Vec3(4, 5, 6);
            t.TransformPosition(v);
            EZ_TEST.VEC3(v, new ez.Vec3((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3));

            v.Set(4, 5, 6);
            t.TransformDirection(v);
            EZ_TEST.VEC3(v, new ez.Vec3((5 * -2), (-6 * 4), (-4 * 2)));
        }

        // ConcatenateRotations / ConcatenateRotationsReverse
        {
            let t = new ez.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(new ez.Vec3(0, 1, 0), ez.Angle.DegreeToRadian(90));
            t.scale.SetAll(2);

            let q = new ez.Quat();
            q.SetFromAxisAndAngle(new ez.Vec3(0, 0, 1), ez.Angle.DegreeToRadian(90));

            let t2 = t.Clone();
            let t4 = t.Clone();
            t2.ConcatenateRotations(q);
            t4.ConcatenateRotationsReverse(q);

            let t3 = t.Clone();
            t3.ConcatenateRotations(q);
            EZ_TEST.BOOL(t2.IsEqual(t3));
            EZ_TEST.BOOL(!t3.IsEqual(t4));

            let a = new ez.Vec3(7, 8, 9);
            let b = a.Clone();
            t2.TransformPosition(b);

            let c = a.Clone();
            q.RotateVec3(c);
            t.TransformPosition(c);

            EZ_TEST.VEC3(b, c);
        }

        // GetAsMat4
        {
            let t = new ez.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(new ez.Vec3(0, 1, 0), ez.Angle.DegreeToRadian(34));
            t.scale.Set(2, -1, 5);

            let m = t.GetAsMat4();

            // reference
            {
                let q = new ez.Quat();
                q.SetFromAxisAndAngle(new ez.Vec3(0, 1, 0), ez.Angle.DegreeToRadian(34));

                let referenceTransform = new ez.Transform();
                referenceTransform.position.Set(1, 2, 3);
                referenceTransform.rotation.SetQuat(q);
                referenceTransform.scale.Set(2, -1, 5);

                let refM = referenceTransform.GetAsMat4();

                EZ_TEST.BOOL(m.IsEqual(refM));
            }

            let p: ez.Vec3[] = [new ez.Vec3(- 4, 0, 0), new ez.Vec3(5, 0, 0), new ez.Vec3(0, -6, 0), new ez.Vec3(0, 7, 0),
            new ez.Vec3(0, 0, -8), new ez.Vec3(0, 0, 9), new ez.Vec3(1, -2, 3), new ez.Vec3(-4, 5, 7)];

            for (let i = 0; i < 8; ++i) {

                let pt = p[i].Clone();
                t.TransformPosition(pt);

                let pm = p[i].Clone();
                m.TransformPosition(pm);

                EZ_TEST.VEC3(pt, pm);
            }
        }

        // SetFromMat4
        {
            let mRot = new ez.Mat3();
            mRot.SetRotationMatrix((new ez.Vec3(1, 2, 3)).GetNormalized(), ez.Angle.DegreeToRadian(42));

            let mTrans = new ez.Mat4();
            mTrans.SetTransformationMatrix(mRot, new ez.Vec3(1, 2, 3));

            let t = new ez.Transform();
            t.SetFromMat4(mTrans);
            EZ_TEST.VEC3(t.position, new ez.Vec3(1, 2, 3), 0);
            EZ_TEST.BOOL(t.rotation.GetAsMat3().IsEqual(mRot, 0.001));
        }
    }

    OnMsgGenericEvent(msg: ez.MsgGenericEvent): void {

        if (msg.Message == "TestTransform") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

