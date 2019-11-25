import __Utils = require("./Utils")
export import Utils = __Utils.Utils;

import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

import __Quat = require("./Quat")
export import Quat = __Quat.Quat;

export class Transform {

    position: Vec3 = new Vec3();
    rotation: Quat = new Quat();
    scale: Vec3 = new Vec3(1, 1, 1);

    Clone(): Transform {
        let clone = new Transform();
        clone.position = this.position;
        clone.rotation = this.rotation;
        clone.scale = this.scale;
        return clone;
    }

    SetTransform(rhs: Transform): void {
        this.position = rhs.position;
        this.rotation = rhs.rotation;
        this.scale = rhs.scale;
    }

    SetIdentity(): void {
        this.position.SetZero();
        this.rotation.SetIdentity();
        this.scale.SetAll(1);
    }

    static IdentityTransform(): Transform {
        return new Transform();
    }

    IsIdentical(rhs: Transform): boolean {
        return this.position.IsIdentical(rhs.position) && this.rotation.IsIdentical(rhs.rotation) && this.scale.IsIdentical(rhs.scale);
    }

    IsEqual(rhs: Transform, epsilon: number): boolean {
        return this.position.IsEqual(rhs.position, epsilon) && this.rotation.IsEqualRotation(rhs.rotation, epsilon) && this.scale.IsEqual(rhs.scale, epsilon);
    }

    Invert(): void {
        this.rotation.Negate();
        this.scale.DivVec3(this.scale);
        this.position.Negate();
        this.position.MulVec3(this.scale);
        this.rotation.RotateVec3(this.position);
    }

    GetInverse(): Transform {
        let invTransform = this.Clone();
        invTransform.Invert();
        return invTransform;
    }

    TransformPosition(pos: Vec3): void {
        pos.MulVec3(this.scale);
        this.rotation.RotateVec3(pos);
        pos.AddVec3(this.position);
    }

    TransformDirection(pos: Vec3): void {
        pos.MulVec3(this.scale);
        this.rotation.RotateVec3(pos);
    }

    Translate(movePos: Vec3): void {
        this.position.AddVec3(movePos);
    }

    // void SetFromMat4(const ezMat4& mat)
    // const ezMat4Template<Type> GetAsMat4() const; // [tested]

    SetLocalTransform(GlobalTransformParent: Transform, GlobalTransformChild: Transform): void {

        let invScale = new Vec3(1, 1, 1);
        invScale.DivVec3(GlobalTransformParent.scale);

        this.position.SetSub(GlobalTransformChild.position, GlobalTransformParent.position);
        GlobalTransformParent.rotation.InvRotateVec3(this.position);
        this.position.MulVec3(invScale);

        this.rotation.SetQuat(GlobalTransformParent.rotation);
        this.rotation.Negate();
        this.rotation.ConcatenateRotations(GlobalTransformChild.rotation);

        this.scale.SetVec3(invScale);
        this.scale.MulVec3(GlobalTransformChild.scale);
    }

    SetGlobalTransform(GlobalTransformParent: Transform, LocalTransformChild: Transform): void {
        this.SetMulTransform(GlobalTransformParent, LocalTransformChild);
    }

    MulTransform(rhs: Transform): void {

        let tmp1 = this.scale.Clone();
        tmp1.MulVec3(rhs.position);
        this.rotation.RotateVec3(tmp1);
        this.position.AddVec3(tmp1);

        this.rotation.ConcatenateRotations(rhs.rotation);

        this.scale.MulVec3(rhs.scale);
    }

    SetMulTransform(lhs: Transform, rhs: Transform): void {
        this.SetTransform(lhs);
        this.MulTransform(rhs);
    }

    ConcatenateRotations(rhs: Quat): void {
        this.rotation.ConcatenateRotations(rhs);
    }

    ConcatenateRotationsReverse(lhs: Quat): void {
        this.rotation.SetConcatenatedRotations(lhs, this.rotation.Clone());
    }
}