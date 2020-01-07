import __Utils = require("./Utils")
export import Utils = __Utils.Utils;

import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

import __Quat = require("./Quat")
export import Quat = __Quat.Quat;

import __Mat4 = require("./Mat4")
export import Mat4 = __Mat4.Mat4;

/**
 * A 'transform' represents a position/translation, rotation and scaling with dedicated values for each.
 */
export class Transform {

    position: Vec3 = new Vec3();     /** Identity translation (none) by default */
    rotation: Quat = new Quat();     /** Identity rotation by default */
    scale: Vec3 = new Vec3(1, 1, 1); /** Identity scaling (one) by default */

    /**
     * Returns a duplicate of this transform.
     */
    Clone(): Transform { // [tested]
        let clone = new Transform();
        clone.position = this.position.Clone();
        clone.rotation = this.rotation.Clone();
        clone.scale = this.scale.Clone();
        return clone;
    }

    /**
     * Copies the transform values from rhs.
     */
    SetTransform(rhs: Transform): void { // [tested]
        this.position = rhs.position.Clone();
        this.rotation = rhs.rotation.Clone();
        this.scale = rhs.scale.Clone();
    }

    /**
     * Sets this to be the identity transform.
     */
    SetIdentity(): void { // [tested]
        this.position.SetZero();
        this.rotation.SetIdentity();
        this.scale.SetAll(1);
    }

    /**
     * Returns an identity transform.
     */
    static IdentityTransform(): Transform { // [tested]
        return new Transform();
    }

    /**
     * Checks whether this and rhs are identical.
     */
    IsIdentical(rhs: Transform): boolean { // [tested]
        return this.position.IsIdentical(rhs.position) && this.rotation.IsIdentical(rhs.rotation) && this.scale.IsIdentical(rhs.scale);
    }

    /**
     * Checks whether this and rhs are approximately equal.
     */
    IsEqual(rhs: Transform, epsilon: number = 0.0001): boolean { // [tested]
        return this.position.IsEqual(rhs.position, epsilon) && this.rotation.IsEqualRotation(rhs.rotation, epsilon) && this.scale.IsEqual(rhs.scale, epsilon);
    }

    /**
     * Inverts the transformation.
     */
    Invert(): void { // [tested]
        this.rotation.Negate();
        this.scale.x = 1.0 / this.scale.x;
        this.scale.y = 1.0 / this.scale.y;
        this.scale.z = 1.0 / this.scale.z;
        this.position.Negate();
        this.position.MulVec3(this.scale);
        this.rotation.RotateVec3(this.position);
    }

    /**
     * Returns the inverse transformation.
     */
    GetInverse(): Transform { // [tested]
        let invTransform = this.Clone();
        invTransform.Invert();
        return invTransform;
    }

    /**
     * Modifies 'pos' in place and treats it like a position vector, ie. with an implied w-component of 1.
     * Thus the translation part of the transform is applied.
     */
    TransformPosition(pos: Vec3): void { // [tested]
        pos.MulVec3(this.scale);
        this.rotation.RotateVec3(pos);
        pos.AddVec3(this.position);
    }

    /**
     * Modifies 'dir' in place and treats it like a direction vector, ie. with an implied w-component of 0.
     * Thus the translation part of the transform is NOT applied, only rotation and scaling.
     */
    TransformDirection(dir: Vec3): void { // [tested]
        dir.MulVec3(this.scale);
        this.rotation.RotateVec3(dir);
    }

    /**
     * Adds more translation to this transform.
     */
    Translate(movePos: Vec3): void { // [tested]
        this.position.AddVec3(movePos);
    }

    /**
     * Sets this transform to be the local transformation needed to get from the parent's transform to the child's.
     */
    SetLocalTransform(globalTransformParent: Transform, globalTransformChild: Transform): void { // [tested]

        let invScale = new Vec3(1, 1, 1);
        invScale.DivVec3(globalTransformParent.scale);

        this.position.SetSub(globalTransformChild.position, globalTransformParent.position);
        globalTransformParent.rotation.InvRotateVec3(this.position);
        this.position.MulVec3(invScale);

        this.rotation.SetQuat(globalTransformParent.rotation);
        this.rotation.Negate();
        this.rotation.ConcatenateRotations(globalTransformChild.rotation);

        this.scale.SetVec3(invScale);
        this.scale.MulVec3(globalTransformChild.scale);
    }

    /**
     * Sets this transform to the global transform, that is reached by applying the child's local transform to the parent's global one.
     */
    SetGlobalTransform(globalTransformParent: Transform, localTransformChild: Transform): void { // [tested]
        this.SetMulTransform(globalTransformParent, localTransformChild);
    }

    /**
     * Sets this transform to be the concatenated transformation of this and rhs.
     *   this = this * rhs
     */
    MulTransform(rhs: Transform): void { // [tested]

        let tmp1 = this.scale.Clone();
        tmp1.MulVec3(rhs.position);
        this.rotation.RotateVec3(tmp1);
        this.position.AddVec3(tmp1);

        this.rotation.ConcatenateRotations(rhs.rotation);

        this.scale.MulVec3(rhs.scale);
    }

    /**
     * Sets this transform to be the concatenated transformation of lhs and rhs.
     *   this = lhs * rhs
     */
    SetMulTransform(lhs: Transform, rhs: Transform): void { // [tested]
        this.SetTransform(lhs);
        this.MulTransform(rhs);
    }

    /**
     * Modifies this rotation to be the concatenation of this and rhs.
     *   this.rotation = this.rotation * rhs
     */
    ConcatenateRotations(rhs: Quat): void { // [tested]
        this.rotation.ConcatenateRotations(rhs);
    }

    /**
     * Modifies this rotation to be the reverse concatenation of this and rhs.
     *   this.rotation = lhs * this.rotation
     */
    ConcatenateRotationsReverse(lhs: Quat): void { // [tested]
        this.rotation.SetConcatenatedRotations(lhs, this.rotation.Clone());
    }

    /**
     * Returns the transformation as a matrix.
     */
    GetAsMat4(): Mat4 { // [tested]
        let result = this.rotation.GetAsMat4();

        result.m_ElementsCM[0] *= this.scale.x;
        result.m_ElementsCM[1] *= this.scale.x;
        result.m_ElementsCM[2] *= this.scale.x;

        result.m_ElementsCM[4] *= this.scale.y;
        result.m_ElementsCM[5] *= this.scale.y;
        result.m_ElementsCM[6] *= this.scale.y;

        result.m_ElementsCM[8] *= this.scale.z;
        result.m_ElementsCM[9] *= this.scale.z;
        result.m_ElementsCM[10] *= this.scale.z;

        result.m_ElementsCM[12] = this.position.x;
        result.m_ElementsCM[13] = this.position.y;
        result.m_ElementsCM[14] = this.position.z;

        return result;
    }

    /**
     * Attempts to extract position, scale and rotation from the matrix. Negative scaling and shearing will get lost in the process.
     */
    SetFromMat4(mat: Mat4): void { // [tested]

        let mRot = mat.GetRotationalPart();

        this.position.SetVec3(mat.GetTranslationVector());
        this.scale.SetVec3(mRot.GetScalingFactors());
        mRot.SetScalingFactors(1, 1, 1);
        this.rotation.SetFromMat3(mRot);
    }

}