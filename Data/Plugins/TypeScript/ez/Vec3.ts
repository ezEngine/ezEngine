import __Utils = require("./Utils")
export import Utils = __Utils.Utils;

import __Angle = require("./Angle")
export import Angle = __Angle.Angle;

import __Vec2 = require("./Vec2")
export import Vec2 = __Vec2.Vec2;

/**
 * A three-component vector type.
 */
export class Vec3 {

    x: number;
    y: number;
    z: number;

    // TODO:
    // GetAngleBetween
    // CreateRandomDeviationX/Y/Z/Normal

    /**
     * Default initializes the vector to all zero.
     */
    constructor(x: number = 0, y: number = 0, z: number = 0) { // [tested]
        this.x = x;
        this.y = y;
        this.z = z;
    }

    /**
     * Returns a duplicate of this vector.
     */
    Clone(): Vec3 { // [tested]
        return new Vec3(this.x, this.y, this.z);
    }

    /**
     * Returns a duplicate Vec2 with the z component removed.
     */
    CloneAsVec2() { // [tested]
        return new Vec2(this.x, this.y);
    }

    /**
     * Returns a vector with all components set to zero.
     */
    static ZeroVector(): Vec3 { // [tested]
        return new Vec3(0, 0, 0);
    }

    /**
     * Returns a vector with all components set to one.
     */
    static OneVector(): Vec3 { // [tested]
        return new Vec3(1, 1, 1);
    }

    /**
     * Returns the vector (1, 0, 0)
     */
    static UnitAxisX(): Vec3 { // [tested]
        return new Vec3(1, 0, 0);
    }

    /**
     * Returns the vector (0, 1, 0)
     */
    static UnitAxisY(): Vec3 { // [tested]
        return new Vec3(0, 1, 0);
    }

    /**
     * Returns the vector (0, 0, 1)
     */
    static UnitAxisZ(): Vec3 { // [tested]
        return new Vec3(0, 0, 1);
    }

    /**
     * Sets all components to the given values.
     */
    Set(x: number, y: number, z: number): void { // [tested]
        this.x = x;
        this.y = y;
        this.z = z;
    }

    /**
     * Copies all component values from rhs.
     */
    SetVec3(rhs: Vec3): void { // [tested]
        this.x = rhs.x;
        this.y = rhs.y;
        this.z = rhs.z;
    }

    /**
     * Sets all components to the value 'val'.
     */
    SetAll(val: number): void { // [tested]
        this.x = val;
        this.y = val;
        this.z = val;
    }

    /**
     * Sets all components to zero.
     */
    SetZero(): void { // [tested]
        this.x = 0;
        this.y = 0;
        this.z = 0;
    }

    /**
     * Returns the squared length of the vector.
     */
    GetLengthSquared(): number { // [tested]
        return this.x * this.x + this.y * this.y + this.z * this.z;
    }

    /**
     * Returns the length of the vector.
     */
    GetLength(): number { // [tested]
        return Math.sqrt(this.x * this.x + this.y * this.y + this.z * this.z);
    }

    /**
     * Computes and returns the length of the vector, and normalizes the vector.
     * This is more efficient than calling GetLength() followed by Normalize().
     */
    GetLengthAndNormalize(): number { // [tested]
        let length = this.GetLength();
        let invLength = 1.0 / length;
        this.x *= invLength;
        this.y *= invLength;
        this.z *= invLength;

        return length;
    }

    /**
     * Normalizes the vector. Afterwards its length will be one.
     * This only works on non-zero vectors. Calling Normalize() on a zero-vector is an error.
     */
    Normalize(): void { // [tested]
        let invLength = 1.0 / this.GetLength();
        this.x *= invLength;
        this.y *= invLength;
        this.z *= invLength;
    }

    /**
     * Returns a normalized duplicate of this vector.
     * Calling this on a zero-vector is an error.
     */
    GetNormalized(): Vec3 { // [tested]
        let norm = this.Clone();
        norm.Normalize();
        return norm;
    }

    /**
     * Normalizes the vector as long as it is not a zero-vector (within the given epsilon).
     * If it is determined to be too close to zero, it is set to 'fallback'.
     * 
     * @param fallback The value to use in case 'this' is too close to zero.
     * @param epsilon The epsilon within this vector is considered to be a zero-vector.
     * @returns true if the vector was normalized regularly, false if the vector was close to zero and 'fallback' was used instead.
     */
    NormalizeIfNotZero(fallback: Vec3 = new Vec3(1, 0, 0), epsilon: number = 0.000001): boolean { // [tested]
        let length = this.GetLength();

        if (length >= -epsilon && length <= epsilon) {
            this.SetVec3(fallback);
            return false;
        }

        this.DivNumber(length);
        return true;
    }

    /**
     * Checks whether all components of this are close to zero.
     */
    IsZero(epsilon: number = 0): boolean { // [tested]
        if (epsilon != 0) {
            return this.x >= -epsilon && this.x <= epsilon &&
                this.y >= -epsilon && this.y <= epsilon &&
                this.z >= -epsilon && this.z <= epsilon;

        }
        else {
            return this.x == 0 && this.y == 0 && this.z == 0;
        }
    }

    /**
     * Checks whether this is normalized within some epsilon.
     */
    IsNormalized(epsilon: number = 0.001): boolean { // [tested]
        let length = this.GetLength();
        return (length >= 1.0 - epsilon) && (length <= 1.0 + epsilon);
    }

    /**
     * Returns a negated duplicate of this.
     */
    GetNegated(): Vec3 { // [tested]
        return new Vec3(-this.x, -this.y, -this.z);
    }

    /**
     * Negates all components of this.
     */
    Negate(): void { // [tested]
        this.x = -this.x;
        this.y = -this.y;
        this.z = -this.z;
    }

    /**
     * Adds rhs component-wise to this.
     */
    AddVec3(rhs: Vec3): void { // [tested]
        this.x += rhs.x;
        this.y += rhs.y;
        this.z += rhs.z;
    }

    /**
     * Subtracts rhs component-wise from this.
     */
    SubVec3(rhs: Vec3): void { // [tested]
        this.x -= rhs.x;
        this.y -= rhs.y;
        this.z -= rhs.z;
    }

    /**
     * Multiplies rhs component-wise into this.
     */
    MulVec3(rhs: Vec3): void { // [tested]
        this.x *= rhs.x;
        this.y *= rhs.y;
        this.z *= rhs.z;
    }

    /**
     * Divides each component of this by rhs.
     */
    DivVec3(rhs: Vec3): void { // [tested]
        this.x /= rhs.x;
        this.y /= rhs.y;
        this.z /= rhs.z;
    }

    /**
     * Multiplies all components of this by 'val'.
     */
    MulNumber(val: number): void { // [tested]
        this.x *= val;
        this.y *= val;
        this.z *= val;
    }

    /**
     * Divides all components of this by 'val'.
     */
    DivNumber(val: number): void { // [tested]
        let invVal = 1.0 / val;
        this.x *= invVal;
        this.y *= invVal;
        this.z *= invVal;
    }

    /**
     * Checks whether this and rhs are exactly identical.
     */
    IsIdentical(rhs: Vec3): boolean { // [tested]
        return this.x == rhs.x && this.y == rhs.y && this.z == rhs.z;
    }

    /**
     * Checks whether this and rhs are approximately equal within a given epsilon.
     */
    IsEqual(rhs: Vec3, epsilon: number = 0.0001): boolean { // [tested]
        return (this.x >= rhs.x - epsilon && this.x <= rhs.x + epsilon) &&
            (this.y >= rhs.y - epsilon && this.y <= rhs.y + epsilon) &&
            (this.z >= rhs.z - epsilon && this.z <= rhs.z + epsilon);
    }

    /**
     * Returns the dot-product between this and rhs.
     */
    Dot(rhs: Vec3): number { // [tested]
        return this.x * rhs.x + this.y * rhs.y + this.z * rhs.z;
    }

    /**
     * Returns the cross-product between this and rhs.
     */
    CrossRH(rhs: Vec3): Vec3 { // [tested]
        return new Vec3(this.y * rhs.z - this.z * rhs.y, this.z * rhs.x - this.x * rhs.z, this.x * rhs.y - this.y * rhs.x);
    }

    /**
     * Sets this to be the cross product between lhs and rhs.
     */
    SetCrossRH(lhs: Vec3, rhs: Vec3): void { // [tested]
        this.x = lhs.y * rhs.z - lhs.z * rhs.y;
        this.y = lhs.z * rhs.x - lhs.x * rhs.z;
        this.z = lhs.x * rhs.y - lhs.y * rhs.x;
    }

    /**
     * Returns a vector consisting of the minimum of the respective components of this and rhs.
     */
    GetCompMin(rhs: Vec3): Vec3 { // [tested]
        return new Vec3(Math.min(this.x, rhs.x), Math.min(this.y, rhs.y), Math.min(this.z, rhs.z));
    }

    /**
     * Returns a vector consisting of the maximum of the respective components of this and rhs.
     */
    GetCompMax(rhs: Vec3): Vec3 { // [tested]
        return new Vec3(Math.max(this.x, rhs.x), Math.max(this.y, rhs.y), Math.max(this.z, rhs.z));
    }

    /**
     * Returns a vector where each component is set to this component's value, clamped to the respective low and high value.
     */
    GetCompClamp(low: Vec3, high: Vec3): Vec3 { // [tested]
        let _x = Math.max(low.x, Math.min(high.x, this.x));
        let _y = Math.max(low.y, Math.min(high.y, this.y));
        let _z = Math.max(low.z, Math.min(high.z, this.z));

        return new Vec3(_x, _y, _z);
    }

    /**
     * Returns a vector with each component being the product of this and rhs.
     */
    GetCompMul(rhs: Vec3): Vec3 { // [tested]
        return new Vec3(this.x * rhs.x, this.y * rhs.y, this.z * rhs.z);
    }

    /**
     * Returns a vector with each component being the division of this and rhs.
     */
    GetCompDiv(rhs: Vec3): Vec3 { // [tested]
        return new Vec3(this.x / rhs.x, this.y / rhs.y, this.z / rhs.z);
    }

    /**
     * Returns a vector with each component set to the absolute value of this vector's respective component.
     */
    GetAbs(): Vec3 { // [tested]
        return new Vec3(Math.abs(this.x), Math.abs(this.y), Math.abs(this.z));
    }

    /**
     * Sets this vector's components to the absolute value of lhs's respective components.
     */
    SetAbs(lhs: Vec3): void { // [tested]
        this.x = Math.abs(lhs.x);
        this.y = Math.abs(lhs.y);
        this.z = Math.abs(lhs.z);
    }

    /**
     * Sets this vector to be the normal of the plane formed by the given three points in space.
     * The points are expected to be in counter-clockwise order to define the 'front' of a triangle.
     * If the points are given in clockwise order, the normal will point in the opposite direction.
     * The points must form a proper triangle, if they are degenerate, the calculation may fail.
     * 
     * @returns true when the normal could be calculated successfully.
     */
    CalculateNormal(v1: Vec3, v2: Vec3, v3: Vec3): boolean { // [tested]
        let tmp1 = new Vec3();
        tmp1.SetSub(v3, v2);

        let tmp2 = new Vec3();
        tmp2.SetSub(v1, v2);

        this.SetCrossRH(tmp1, tmp2);
        return this.NormalizeIfNotZero();
    }

    /**
     * Adjusts this vector such that it is orthogonal to the given normal.
     * The operation may change the length of this vector.
     */
    MakeOrthogonalTo(normal: Vec3): void { // [tested]
        let ortho = normal.CrossRH(this);
        this.SetCrossRH(ortho, normal);
    }

    /**
     * Returns an arbitrary vector that is orthogonal to this vector.
     */
    GetOrthogonalVector(): Vec3 { // [tested]
        if (Math.abs(this.y) < 0.999) {
            return this.CrossRH(new Vec3(0, 1, 0));
        }

        return this.CrossRH(new Vec3(1, 0, 0));
    }

    /**
     * Returns a vector that is this vector reflected along the given normal.
     */
    GetReflectedVector(normal: Vec3): Vec3 { // [tested]
        let res = this.Clone();
        let tmp = normal.Clone();
        tmp.MulNumber(this.Dot(normal) * 2.0);
        res.SubVec3(tmp);
        return res;
    }

    /**
     * Sets this vector to be the addition of lhs and rhs.
     */
    SetAdd(lhs: Vec3, rhs: Vec3): void { // [tested]
        this.x = lhs.x + rhs.x;
        this.y = lhs.y + rhs.y;
        this.z = lhs.z + rhs.z;
    }

    /**
     * Sets this vector to be the subtraction of lhs and rhs.
     */
    SetSub(lhs: Vec3, rhs: Vec3): void { // [tested]
        this.x = lhs.x - rhs.x;
        this.y = lhs.y - rhs.y;
        this.z = lhs.z - rhs.z;
    }

    /**
     * Sets this vector to be the product of lhs and rhs.
     */
    SetMul(lhs: Vec3, rhs: number): void { // [tested]
        this.x = lhs.x * rhs;
        this.y = lhs.y * rhs;
        this.z = lhs.z * rhs;
    }

    /**
     * Sets this vector to be the division of lhs and rhs.
     */
    SetDiv(lhs: Vec3, rhs: number): void { // [tested]
        let invRhs = 1.0 / rhs;
        this.x = lhs.x * invRhs;
        this.y = lhs.y * invRhs;
        this.z = lhs.z * invRhs;
    }

    /**
     * Attempts to modify this vector such that it has the desired length.
     * Requires that this vector is not zero.
     * 
     * @returns true if the vector's length could be changed as desired.
     */
    SetLength(length: number, epsilon: number): boolean { // [tested]
        if (!this.NormalizeIfNotZero(Vec3.ZeroVector(), epsilon))
            return false;

        this.MulNumber(length);
        return true;
    }

    /**
     * Sets this vector to the linear interpolation between lhs and rhs.
     * @param lerpFactor Factor between 0 and 1 that specifies how much to interpolate.
     */    
    SetLerp(lhs: Vec3, rhs: Vec3, lerpFactor: number) {
        this.SetSub(rhs, lhs);
        this.MulNumber(lerpFactor);
        this.AddVec3(lhs);
    }

    /**
     * Returns a random point inside a sphere of radius 1 around the origin.
     */
    static CreateRandomPointInSphere(): Vec3 { // [tested]
        let px: number, py: number, pz: number;
        let len: number = 0.0;

        do {
            px = Math.random() * 2.0 - 1.0;
            py = Math.random() * 2.0 - 1.0;
            pz = Math.random() * 2.0 - 1.0;

            len = (px * px) + (py * py) + (pz * pz);
        } while (len > 1.0 || len <= 0.000001); // prevent the exact center

        return new Vec3(px, py, pz);
    }

    /**
     * Returns a random direction vector.
     */
    static CreateRandomDirection(): Vec3 { // [tested]
        let res = Vec3.CreateRandomPointInSphere();
        res.Normalize();
        return res;
    }

}