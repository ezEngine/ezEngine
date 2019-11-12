import __Math = require("./Math")
export import ezMath = __Math.ezMath;

import __Angle = require("./Angle")
export import Angle = __Angle.Angle;

import __Vec2 = require("./Vec2")
export import Vec2 = __Vec2.Vec2;

export class Vec3 {
    x: number;
    y: number;
    z: number;

    constructor(x: number = 0, y: number = 0, z: number = 0) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    Clone(): Vec3 {
        return new Vec3(this.x, this.y, this.z);
    }

    CloneAsVec2() {
        return new Vec2(this.x, this.y);
    }

    static ZeroVector(): Vec3 {
        return new Vec3(0, 0, 0);
    }

    static OneVector(): Vec3 {
        return new Vec3(1, 1, 1);
    }

    static UnitAxisX(): Vec3 {
        return new Vec3(1, 0, 0);
    }

    static UnitAxisY(): Vec3 {
        return new Vec3(0, 1, 0);
    }

    static UnitAxisZ(): Vec3 {
        return new Vec3(0, 0, 1);
    }

    Set(x: number, y: number, z: number): void {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    SetVec3(rhs: Vec3): void {
        this.x = rhs.x;
        this.y = rhs.y;
        this.z = rhs.z;
    }

    SetAll(val: number): void {
        this.x = val;
        this.y = val;
        this.z = val;
    }

    SetZero(): void {
        this.x = 0;
        this.y = 0;
        this.z = 0;
    }

    GetLengthSquared(): number {
        return this.x * this.x + this.y * this.y + this.z * this.z;
    }

    GetLength(): number {
        return Math.sqrt(this.x * this.x + this.y * this.y + this.z * this.z);
    }

    GetLengthAndNormalize(): number {
        let length = this.GetLength();
        let invLength = 1.0 / length;
        this.x *= invLength;
        this.y *= invLength;
        this.z *= invLength;

        return length;
    }

    Normalize(): void {
        let invLength = 1.0 / this.GetLength();
        this.x *= invLength;
        this.y *= invLength;
        this.z *= invLength;
    }

    GetNormalized(): Vec3 {
        let norm = this.Clone();
        norm.Normalize();
        return norm;
    }

    NormalizeIfNotZero(fallback: Vec3 = new Vec3(1, 0, 0), epsilon: number = 0.000001): boolean {
        let length = this.GetLength();

        if (length >= -epsilon && length <= epsilon) {
            this.SetVec3(fallback);
            return false;
        }

        this.DivNumber(length);
        return true;
    }

    IsZero(epsilon: number = 0): boolean {
        if (epsilon != 0) {
            return this.x >= -epsilon && this.x <= epsilon &&
                this.y >= -epsilon && this.y <= epsilon &&
                this.z >= -epsilon && this.z <= epsilon;

        }
        else {
            return this.x == 0 && this.y == 0 && this.z == 0;
        }
    }

    IsNormalized(epsilon: number = 0.001): boolean {
        let length = this.GetLength();
        return (length >= 1.0 - epsilon) && (length <= 1.0 - epsilon);
    }

    GetNegated(): Vec3 {
        return new Vec3(-this.x, -this.y, -this.z);
    }

    Negate(): void {
        this.x = -this.x;
        this.y = -this.y;
        this.z = -this.z;
    }

    AddVec3(rhs: Vec3): void {
        this.x += rhs.x;
        this.y += rhs.y;
        this.z += rhs.z;
    }

    SubVec3(rhs: Vec3): void {
        this.x -= rhs.x;
        this.y -= rhs.y;
        this.z -= rhs.z;
    }

    MulVec3(rhs: Vec3): void {
        this.x *= rhs.x;
        this.y *= rhs.y;
        this.z *= rhs.z;
    }

    DivVec3(rhs: Vec3): void {
        this.x /= rhs.x;
        this.y /= rhs.y;
        this.z /= rhs.z;
    }

    MulNumber(val: number): void {
        this.x *= val;
        this.y *= val;
        this.z *= val;
    }

    DivNumber(val: number): void {
        let invVal = 1.0 / val;
        this.x *= invVal;
        this.y *= invVal;
        this.z *= invVal;
    }

    IsIdentical(rhs: Vec3): boolean {
        return this.x == rhs.x && this.y == rhs.y && this.z == rhs.z;
    }

    IsEqual(rhs: Vec3, epsilon: number): boolean {
        return (this.x >= rhs.x - epsilon && this.x <= rhs.x + epsilon) &&
            (this.y >= rhs.y - epsilon && this.y <= rhs.y + epsilon) &&
            (this.z >= rhs.z - epsilon && this.z <= rhs.z + epsilon);
    }

    // GetAngleBetween

    Dot(rhs: Vec3): number {
        return this.x * rhs.x + this.y * rhs.y + this.z * rhs.z;
    }

    CrossRH(rhs: Vec3): Vec3 {
        return new Vec3(this.y * rhs.z - this.z * rhs.y, this.z * rhs.x - this.x * rhs.z, this.x * rhs.y - this.y * rhs.x);
    }

    SetCrossRH(lhs: Vec3, rhs: Vec3): void {
        this.x = lhs.y * rhs.z - lhs.z * rhs.y;
        this.y = lhs.z * rhs.x - lhs.x * rhs.z;
        this.z = lhs.x * rhs.y - lhs.y * rhs.x;
    }

    GetCompMin(rhs: Vec3): Vec3 {
        return new Vec3(Math.min(this.x, rhs.x), Math.min(this.y, rhs.y), Math.min(this.z, rhs.z));
    }

    GetCompMax(rhs: Vec3): Vec3 {
        return new Vec3(Math.max(this.x, rhs.x), Math.max(this.y, rhs.y), Math.max(this.z, rhs.z));
    }

    GetCompClamp(low: Vec3, high: Vec3): Vec3 {
        let _x = Math.max(low.x, Math.min(high.x, this.x));
        let _y = Math.max(low.y, Math.min(high.y, this.y));
        let _z = Math.max(low.z, Math.min(high.z, this.z));

        return new Vec3(_x, _y, _z);
    }

    GetCompMul(rhs: Vec3): Vec3 {
        this.MulVec3
        return new Vec3(this.x * rhs.x, this.y * rhs.y, this.z * rhs.z);
    }

    GetCompDiv(rhs: Vec3): Vec3 {
        return new Vec3(this.x / rhs.x, this.y / rhs.y, this.z / rhs.z);
    }

    GetAbs(): Vec3 {
        return new Vec3(Math.abs(this.x), Math.abs(this.y), Math.abs(this.z));
    }

    SetAbs(lhs: Vec3): void {
        this.x = Math.abs(lhs.x);
        this.y = Math.abs(lhs.y);
        this.z = Math.abs(lhs.z);
    }

    CalculateNormal(v1: Vec3, v2: Vec3, v3: Vec3): boolean {
        let tmp1: Vec3;
        tmp1.SetSub(v3, v2);

        let tmp2: Vec3;
        tmp2.SetSub(v1, v2);

        this.SetCrossRH(tmp1, tmp2);
        return this.NormalizeIfNotZero();
    }

    MakeOrthogonalTo(normal: Vec3): void {
        let ortho = normal.CrossRH(this);
        this.SetCrossRH(ortho, normal);
    }

    GetOrthogonalVector(): Vec3 {
        if (Math.abs(this.y) < 0.999) {
            return this.CrossRH(new Vec3(0, 1, 0));
        }

        return this.CrossRH(new Vec3(1, 0, 0));
    }

    GetReflectedVector(normal: Vec3): Vec3 {
        let res = this.Clone();
        let tmp = normal.Clone();
        tmp.MulNumber(this.Dot(normal) * 2.0);
        res.SubVec3(tmp);
        return res;
    }

    SetAdd(lhs: Vec3, rhs: Vec3): void {
        this.x = lhs.x + rhs.x;
        this.y = lhs.y + rhs.y;
        this.z = lhs.z + rhs.z;
    }

    SetSub(lhs: Vec3, rhs: Vec3): void {
        this.x = lhs.x - rhs.x;
        this.y = lhs.y - rhs.y;
        this.z = lhs.z - rhs.z;
    }

    SetMul(lhs: Vec3, rhs: number): void {
        this.x = lhs.x * rhs;
        this.y = lhs.y * rhs;
        this.z = lhs.z * rhs;
    }

    SetDiv(lhs: Vec3, rhs: number): void {
        let invRhs = 1.0 / rhs;
        this.x = lhs.x * invRhs;
        this.y = lhs.y * invRhs;
        this.z = lhs.z * invRhs;
    }

    static CreateRandomPointInSphere(): Vec3 {
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

    static CreateRandomDirection(): Vec3 {
        let res = Vec3.CreateRandomPointInSphere();
        res.Normalize();
        return res;
    }

    // CreateRandomDeviationX/Y/Z/Normal
}