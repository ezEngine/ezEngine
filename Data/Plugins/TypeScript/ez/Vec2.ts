import __Utils = require("./Utils")
export import Utils = __Utils.Utils;

import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

/**
 * A two-component vector type.
 */
export class Vec2 {

    x: number;
    y: number;

    // TODO:
    // GetAngleBetween

    /**
     * Default initializes the vector to all zero.
     */
    constructor(x: number = 0, y: number = 0) { // [tested]
        this.x = x;
        this.y = y;
    }

    /**
     * Returns a duplicate of this vector.
     */
    Clone(): Vec2 { // [tested]
        return new Vec2(this.x, this.y);
    }

    /**
     * Returns a duplicate of this as a Vec3, with the provided value as z.
     */
    CloneAsVec3(z: number = 0): Vec3 { // [NOT tested (fails to compile)]
        return new Vec3(this.x, this.y, z);
    }

    /**
     * Returns a vector with all components set to zero.
     */
    static ZeroVector(): Vec2 { // [tested]
        return new Vec2(0, 0);
    }

    /**
     * Returns a vector with all components set to one.
     */
    static OneVector(): Vec2 { // [tested]
        return new Vec2(1, 1);
    }

    /**
     * Returns the vector (1, 0)
     */
    static UnitAxisX(): Vec2 { // [tested]
        return new Vec2(1, 0);
    }

    /**
     * Returns the vector (0, 1)
     */
    static UnitAxisY(): Vec2 { // [tested]
        return new Vec2(0, 1);
    }

    /**
     * Sets all components to the given values.
     */
    Set(x: number, y: number): void { // [tested]
        this.x = x;
        this.y = y;
    }

    /**
     * Copies all component values from rhs.
     */
    SetVec2(rhs: Vec2): void { // [tested]
        this.x = rhs.x;
        this.y = rhs.y;
    }

    /**
     * Sets all components to the value 'val'.
     */
    SetAll(val: number): void { // [tested]
        this.x = val;
        this.y = val;
    }

    /**
     * Sets all components to zero.
     */
    SetZero(): void { // [tested]
        this.x = 0;
        this.y = 0;
    }

    /**
     * Returns the squared length of the vector.
     */
    GetLengthSquared(): number { // [tested]
        return this.x * this.x + this.y * this.y;
    }

    /**
     * Returns the length of the vector.
     */
    GetLength(): number { // [tested]
        return Math.sqrt(this.x * this.x + this.y * this.y);
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
    }

    /**
     * Returns a normalized duplicate of this vector.
     * Calling this on a zero-vector is an error.
     */
    GetNormalized(): Vec2 { // [tested]
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
    NormalizeIfNotZero(fallback: Vec2 = new Vec2(1, 0), epsilon: number = 0.000001): boolean { // [tested]
        let length = this.GetLength();

        if (length >= -epsilon && length <= epsilon) {
            this.SetVec2(fallback);
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
                this.y >= -epsilon && this.y <= epsilon;

        }
        else {
            return this.x == 0 && this.y == 0;
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
    GetNegated(): Vec2 { // [tested]
        return new Vec2(-this.x, -this.y);
    }

    /**
     * Negates all components of this.
     */
    Negate(): void { // [tested]
        this.x = -this.x;
        this.y = -this.y;
    }

    /**
     * Adds rhs component-wise to this.
     */
    AddVec2(rhs: Vec2): void { // [tested]
        this.x += rhs.x;
        this.y += rhs.y;
    }

    /**
     * Subtracts rhs component-wise from this.
     */
    SubVec2(rhs: Vec2): void { // [tested]
        this.x -= rhs.x;
        this.y -= rhs.y;
    }

    /**
     * Multiplies rhs component-wise into this.
     */
    MulVec2(rhs: Vec2): void { // [tested]
        this.x *= rhs.x;
        this.y *= rhs.y;
    }

    /**
     * Divides each component of this by rhs.
     */
    DivVec2(rhs: Vec2): void { // [tested]
        this.x /= rhs.x;
        this.y /= rhs.y;
    }

    /**
     * Multiplies all components of this by 'val'.
     */
    MulNumber(val: number): void { // [tested]
        this.x *= val;
        this.y *= val;
    }

    /**
     * Divides all components of this by 'val'.
     */
    DivNumber(val: number): void { // [tested]
        let invVal = 1.0 / val;
        this.x *= invVal;
        this.y *= invVal;
    }

    /**
     * Checks whether this and rhs are exactly identical.
     */
    IsIdentical(rhs: Vec2): boolean { // [tested]
        return this.x == rhs.x && this.y == rhs.y;
    }

    /**
     * Checks whether this and rhs are approximately equal within a given epsilon.
     */
    IsEqual(rhs: Vec2, epsilon: number): boolean { // [tested]
        return (this.x >= rhs.x - epsilon && this.x <= rhs.x + epsilon) &&
            (this.y >= rhs.y - epsilon && this.y <= rhs.y + epsilon);
    }

    /**
     * Returns the dot-product between this and rhs.
     */
    Dot(rhs: Vec2): number { // [tested]
        return this.x * rhs.x + this.y * rhs.y;
    }

    /**
     * Returns a vector consisting of the minimum of the respective components of this and rhs.
     */
    GetCompMin(rhs: Vec2): Vec2 { // [tested]
        return new Vec2(Math.min(this.x, rhs.x), Math.min(this.y, rhs.y));
    }

    /**
     * Returns a vector consisting of the maximum of the respective components of this and rhs.
     */
    GetCompMax(rhs: Vec2): Vec2 { // [tested]
        return new Vec2(Math.max(this.x, rhs.x), Math.max(this.y, rhs.y));
    }

    /**
     * Returns a vector where each component is set to this component's value, clamped to the respective low and high value.
     */
    GetCompClamp(low: Vec2, high: Vec2): Vec2 { // [tested]
        let _x = Math.max(low.x, Math.min(high.x, this.x));
        let _y = Math.max(low.y, Math.min(high.y, this.y));

        return new Vec2(_x, _y);
    }

    /**
     * Returns a vector with each component being the product of this and rhs.
     */
    GetCompMul(rhs: Vec2): Vec2 { // [tested]
        return new Vec2(this.x * rhs.x, this.y * rhs.y);
    }

    /**
     * Returns a vector with each component being the division of this and rhs.
     */
    GetCompDiv(rhs: Vec2): Vec2 { // [tested]
        return new Vec2(this.x / rhs.x, this.y / rhs.y);
    }

    /**
     * Returns a vector with each component set to the absolute value of this vector's respective component.
     */
    GetAbs(): Vec2 { // [tested]
        return new Vec2(Math.abs(this.x), Math.abs(this.y));
    }

    /**
     * Sets this vector's components to the absolute value of lhs's respective components.
     */
    SetAbs(lhs: Vec2): void { // [tested]
        this.x = Math.abs(lhs.x);
        this.y = Math.abs(lhs.y);
    }

    /**
     * Returns a vector that is this vector reflected along the given normal.
     */
    GetReflectedVector(normal: Vec2): Vec2 { // [tested]
        let res = this.Clone();
        let tmp = normal.Clone();
        tmp.MulNumber(this.Dot(normal) * 2.0);
        res.SubVec2(tmp);
        return res;
    }

    /**
     * Sets this vector to be the addition of lhs and rhs.
     */
    SetAdd(lhs: Vec2, rhs: Vec2): void { // [tested]
        this.x = lhs.x + rhs.x;
        this.y = lhs.y + rhs.y;
    }

    /**
     * Sets this vector to be the subtraction of lhs and rhs.
     */
    SetSub(lhs: Vec2, rhs: Vec2): void { // [tested]
        this.x = lhs.x - rhs.x;
        this.y = lhs.y - rhs.y;
    }

    /**
     * Sets this vector to be the product of lhs and rhs.
     */
    SetMul(lhs: Vec2, rhs: number): void { // [tested]
        this.x = lhs.x * rhs;
        this.y = lhs.y * rhs;
    }

    /**
     * Sets this vector to be the division of lhs and rhs.
     */
    SetDiv(lhs: Vec2, rhs: number): void { // [tested]
        let invRhs = 1.0 / rhs;
        this.x = lhs.x * invRhs;
        this.y = lhs.y * invRhs;
    }

    /**
     * Returns a random point inside a circle of radius 1 around the origin.
     */
    static CreateRandomPointInCircle(): Vec2 { // [tested]
        let px: number, py: number;
        let len: number = 0.0;

        do {
            px = Math.random() * 2.0 - 1.0;
            py = Math.random() * 2.0 - 1.0;

            len = (px * px) + (py * py);
        } while (len > 1.0 || len <= 0.000001); // prevent the exact center

        return new Vec2(px, py);
    }

    /**
     * Returns a random direction vector.
     */
    static CreateRandomDirection(): Vec2 { // [tested]
        let res = Vec2.CreateRandomPointInCircle();
        res.Normalize();
        return res;
    }
}