import __Math = require("./Math")
export import ezMath = __Math.ezMath;

import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

export class Vec2 {
    x: number;
    y: number;

    constructor(x: number = 0, y: number = 0) {
        this.x = x;
        this.y = y;
    }

    Clone(): Vec2 {
        return new Vec2(this.x, this.y);
    }

    CloneAsVec3(z: number = 0) {
        return new Vec3(this.x, this.y, z);
    }

    static ZeroVector(): Vec2 {
        return new Vec2(0, 0);
    }

    static OneVector(): Vec2 {
        return new Vec2(1, 1);
    }

    static UnitAxisX(): Vec2 {
        return new Vec2(1, 0);
    }

    static UnitAxisY(): Vec2 {
        return new Vec2(0, 1);
    }

    Set(x: number, y: number): void {
        this.x = x;
        this.y = y;
    }

    SetVec2(rhs: Vec2): void {
        this.x = rhs.x;
        this.y = rhs.y;
    }

    SetAll(val: number): void {
        this.x = val;
        this.y = val;
    }

    SetZero(): void {
        this.x = 0;
        this.y = 0;
    }

    GetLengthSquared(): number {
        return this.x * this.x + this.y * this.y;
    }

    GetLength(): number {
        return Math.sqrt(this.x * this.x + this.y * this.y);
    }

    GetLengthAndNormalize(): number {
        let length = this.GetLength();
        let invLength = 1.0 / length;
        this.x *= invLength;
        this.y *= invLength;

        return length;
    }

    Normalize(): void {
        let invLength = 1.0 / this.GetLength();
        this.x *= invLength;
        this.y *= invLength;
    }

    GetNormalized(): Vec2 {
        let norm = this.Clone();
        norm.Normalize();
        return norm;
    }

    NormalizeIfNotZero(fallback: Vec2 = new Vec2(1, 0), epsilon: number = 0.000001): boolean {
        let length = this.GetLength();

        if (length >= -epsilon && length <= epsilon) {
            this.SetVec2(fallback);
            return false;
        }

        this.DivNumber(length);
        return true;
    }

    IsZero(epsilon: number = 0): boolean {
        if (epsilon != 0) {
            return this.x >= -epsilon && this.x <= epsilon &&
                this.y >= -epsilon && this.y <= epsilon;

        }
        else {
            return this.x == 0 && this.y == 0;
        }
    }

    IsNormalized(epsilon: number = 0.001): boolean {
        let length = this.GetLength();
        return (length >= 1.0 - epsilon) && (length <= 1.0 - epsilon);
    }

    GetNegated(): Vec2 {
        return new Vec2(-this.x, -this.y);
    }

    Negate(): void {
        this.x = -this.x;
        this.y = -this.y;
    }

    AddVec2(rhs: Vec2): void {
        this.x += rhs.x;
        this.y += rhs.y;
    }

    SubVec2(rhs: Vec2): void {
        this.x -= rhs.x;
        this.y -= rhs.y;
    }

    MulVec2(rhs: Vec2): void {
        this.x *= rhs.x;
        this.y *= rhs.y;
    }

    DivVec2(rhs: Vec2): void {
        this.x /= rhs.x;
        this.y /= rhs.y;
    }

    MulNumber(val: number): void {
        this.x *= val;
        this.y *= val;
    }

    DivNumber(val: number): void {
        let invVal = 1.0 / val;
        this.x *= invVal;
        this.y *= invVal;
    }

    IsIdentical(rhs: Vec2): boolean {
        return this.x == rhs.x && this.y == rhs.y;
    }

    IsEqual(rhs: Vec2, epsilon: number): boolean {
        return (this.x >= rhs.x - epsilon && this.x <= rhs.x + epsilon) &&
            (this.y >= rhs.y - epsilon && this.y <= rhs.y + epsilon);
    }

    // GetAngleBetween

    Dot(rhs: Vec2): number {
        return this.x * rhs.x + this.y * rhs.y;
    }

    GetCompMin(rhs: Vec2): Vec2 {
        return new Vec2(Math.min(this.x, rhs.x), Math.min(this.y, rhs.y));
    }

    GetCompMax(rhs: Vec2): Vec2 {
        return new Vec2(Math.max(this.x, rhs.x), Math.max(this.y, rhs.y));
    }

    GetCompClamp(low: Vec2, high: Vec2): Vec2 {
        let _x = Math.max(low.x, Math.min(high.x, this.x));
        let _y = Math.max(low.y, Math.min(high.y, this.y));

        return new Vec2(_x, _y);
    }

    GetCompMul(rhs: Vec2): Vec2 {
        this.MulVec2
        return new Vec2(this.x * rhs.x, this.y * rhs.y);
    }

    GetCompDiv(rhs: Vec2): Vec2 {
        return new Vec2(this.x / rhs.x, this.y / rhs.y);
    }

    GetAbs(): Vec2 {
        return new Vec2(Math.abs(this.x), Math.abs(this.y));
    }

    SetAbs(lhs: Vec2): void {
        this.x = Math.abs(lhs.x);
        this.y = Math.abs(lhs.y);
    }

    GetReflectedVector(normal: Vec2): Vec2 {
        let res = this.Clone();
        let tmp = normal.Clone();
        tmp.MulNumber(this.Dot(normal) * 2.0);
        res.SubVec2(tmp);
        return res;
    }

    SetAdd(lhs: Vec2, rhs: Vec2): void {
        this.x = lhs.x + rhs.x;
        this.y = lhs.y + rhs.y;
    }

    SetSub(lhs: Vec2, rhs: Vec2): void {
        this.x = lhs.x - rhs.x;
        this.y = lhs.y - rhs.y;
    }

    SetMul(lhs: Vec2, rhs: number): void {
        this.x = lhs.x * rhs;
        this.y = lhs.y * rhs;
    }

    SetDiv(lhs: Vec2, rhs: number): void {
        let invRhs = 1.0 / rhs;
        this.x = lhs.x * invRhs;
        this.y = lhs.y * invRhs;
    }

    static CreateRandomPointInCircle(): Vec2 {
        let px: number, py: number;
        let len: number = 0.0;

        do {
            px = Math.random() * 2.0 - 1.0;
            py = Math.random() * 2.0 - 1.0;

            len = (px * px) + (py * py);
        } while (len > 1.0 || len <= 0.000001); // prevent the exact center

        return new Vec2(px, py);
    }

    static CreateRandomDirection(): Vec2 {
        let res = Vec2.CreateRandomPointInCircle();
        res.Normalize();
        return res;
    }
}