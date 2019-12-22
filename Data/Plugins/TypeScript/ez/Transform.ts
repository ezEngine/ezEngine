import __Utils = require("./Utils")
export import Utils = __Utils.Utils;

import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

import __Quat = require("./Quat")
export import Quat = __Quat.Quat;

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
    Clone(): Transform {
        let clone = new Transform();
        clone.position = this.position;
        clone.rotation = this.rotation;
        clone.scale = this.scale;
        return clone;
    }

    /**
     * Copies the transform values from rhs.
     */
    SetTransform(rhs: Transform): void {
        this.position = rhs.position;
        this.rotation = rhs.rotation;
        this.scale = rhs.scale;
    }

    /**
     * Sets this to be the identity transform.
     */
    SetIdentity(): void {
        this.position.SetZero();
        this.rotation.SetIdentity();
        this.scale.SetAll(1);
    }

    /**
     * Returns an identity transform.
     */
    static IdentityTransform(): Transform {
        return new Transform();
    }

    /**
     * Checks whether this and rhs are identical.
     */
    IsIdentical(rhs: Transform): boolean {
        return this.position.IsIdentical(rhs.position) && this.rotation.IsIdentical(rhs.rotation) && this.scale.IsIdentical(rhs.scale);
    }

    /**
     * Checks whether this and rhs are approximately equal.
     */
    IsEqual(rhs: Transform, epsilon: number): boolean {
        return this.position.IsEqual(rhs.position, epsilon) && this.rotation.IsEqualRotation(rhs.rotation, epsilon) && this.scale.IsEqual(rhs.scale, epsilon);
    }

    /**
     * Inverts the transformation.
     */
    Invert(): void {
        this.rotation.Negate();
        this.scale.DivVec3(this.scale);
        this.position.Negate();
        this.position.MulVec3(this.scale);
        this.rotation.RotateVec3(this.position);
    }

    /**
     * Returns the inverse transformation.
     */
    GetInverse(): Transform {
        let invTransform = this.Clone();
        invTransform.Invert();
        return invTransform;
    }

    /**
     * Modifies 'pos' in place and treats it like a position vector, ie. with an implied w-component of 1.
     * Thus the translation part of the transform is applied.
     */
    TransformPosition(pos: Vec3): void {
        pos.MulVec3(this.scale);
        this.rotation.RotateVec3(pos);
        pos.AddVec3(this.position);
    }

    /**
     * Modifies 'dir' in place and treats it like a direction vector, ie. with an implied w-component of 0.
     * Thus the translation part of the transform is NOT applied, only rotation and scaling.
     */
    TransformDirection(dir: Vec3): void {
        dir.MulVec3(this.scale);
        this.rotation.RotateVec3(dir);
    }

    /**
     * Adds more translation to this transform.
     */
    Translate(movePos: Vec3): void {
        this.position.AddVec3(movePos);
    }

    // TODO:
    // SetFromMat4(m: Mat4): void;
    // GetAsMat4(): Mat4;

    /**
     * Sets this transform to be the local transformation needed to get from the parent's transform to the child's.
     */
    SetLocalTransform(globalTransformParent: Transform, globalTransformChild: Transform): void {

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
    SetGlobalTransform(globalTransformParent: Transform, localTransformChild: Transform): void {
        this.SetMulTransform(globalTransformParent, localTransformChild);
    }

    /**
     * Sets this transform to be the concatenated transformation of this and rhs.
     *   this = this * rhs
     */
    MulTransform(rhs: Transform): void {

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
    SetMulTransform(lhs: Transform, rhs: Transform): void {
        this.SetTransform(lhs);
        this.MulTransform(rhs);
    }

    /**
     * Modifies this rotation to be the concatenation of this and rhs.
     *   this.rotation = this.rotation * rhs
     */
    ConcatenateRotations(rhs: Quat): void {
        this.rotation.ConcatenateRotations(rhs);
    }

    /**
     * Modifies this rotation to be the reverse concatenation of this and rhs.
     *   this.rotation = lhs * this.rotation
     */
    ConcatenateRotationsReverse(lhs: Quat): void {
        this.rotation.SetConcatenatedRotations(lhs, this.rotation.Clone());
    }
}