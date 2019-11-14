import __Math = require("./Math")
export import ezMath = __Math.ezMath;

import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

export class Mat3 {
    m_ElementsCM: number[] = [
        1, 0, 0,
        0, 1, 0,
        0, 0, 1];

    constructor(c1r1: number = 1, c2r1: number = 0, c3r1: number = 0, c1r2: number = 0, c2r2: number = 1, c3r2: number = 0, c1r3: number = 0, c2r3: number = 0, c3r3: number = 1) {
        this.SetElements(c1r1, c2r1, c3r1, c1r2, c2r2, c1r3, c1r3, c2r3, c3r3);
    }

    Clone(): Mat3 {
        let c = new Mat3();
        c.SetMat3(this);
        return c;
    }

    SetMat3(m: Mat3): void {
        for (var i = 0; i < 9; ++i) {
            this.m_ElementsCM[i] = m[i];
        }
    }

    GetElement(column: number, row: number): number {
        return this.m_ElementsCM[column * 3 + row];
    }

    SetElement(column: number, row: number, value: number): void {
        this.m_ElementsCM[column * 3 + row] = value;
    }

    SetElements(c1r1: number, c2r1: number, c3r1: number, c1r2: number, c2r2: number, c3r2: number, c1r3: number, c2r3: number, c3r3: number): void {
        this.m_ElementsCM[0] = c1r1;
        this.m_ElementsCM[3] = c2r1;
        this.m_ElementsCM[6] = c3r1;

        this.m_ElementsCM[1] = c1r2;
        this.m_ElementsCM[4] = c2r2;
        this.m_ElementsCM[7] = c3r2;

        this.m_ElementsCM[2] = c1r3;
        this.m_ElementsCM[5] = c2r3;
        this.m_ElementsCM[8] = c3r3;
    }

    SetZero(): void {
        for (var i = 0; i < 9; ++i) {
            this.m_ElementsCM[i] = 0;
        }
    }

    SetIdentity(): void {
        this.m_ElementsCM[0] = 1;
        this.m_ElementsCM[3] = 0;
        this.m_ElementsCM[6] = 0;

        this.m_ElementsCM[1] = 0;
        this.m_ElementsCM[4] = 1;
        this.m_ElementsCM[7] = 0;

        this.m_ElementsCM[2] = 0;
        this.m_ElementsCM[5] = 0;
        this.m_ElementsCM[8] = 1;
    }

    SetScalingMatrix(scale: Vec3): void {
        this.SetElements(
            scale.x, 0, 0,
            0, scale.y, 0,
            0, 0, scale.z);
    }

    SetRotationMatrixX(radians: number): void {
        const fSin = Math.sin(radians);
        const fCos = Math.cos(radians);

        this.SetElements(
            1, 0, 0,
            0, fCos, -fSin,
            0, fSin, fCos);
    }

    SetRotationMatrixY(radians: number): void {
        const fSin = Math.sin(radians);
        const fCos = Math.cos(radians);

        this.SetElements(
            fCos, 0, fSin,
            0, 1, 0,
            -fSin, 0, fCos);
    }

    SetRotationMatrixZ(radians: number): void {
        const fSin = Math.sin(radians);
        const fCos = Math.cos(radians);

        this.SetElements(
            fCos, -fSin, 0,
            fSin, fCos, 0,
            0, 0, 1);
    }

    SetRotationMatrix(axis: Vec3, radians: number): void {

        const cos = Math.cos(radians);
        const sin = Math.sin(radians);
        const oneminuscos = 1 - cos;

        const xy = axis.x * axis.y;
        const xz = axis.x * axis.z;
        const yz = axis.y * axis.z;

        const xsin = axis.x * sin;
        const ysin = axis.y * sin;
        const zsin = axis.z * sin;

        const onecos_xy = oneminuscos * xy;
        const onecos_xz = oneminuscos * xz;
        const onecos_yz = oneminuscos * yz;

        //Column 1
        this.m_ElementsCM[0] = cos + (oneminuscos * (axis.x * axis.x));
        this.m_ElementsCM[1] = onecos_xy + zsin;
        this.m_ElementsCM[2] = onecos_xz - ysin;

        //Column 2
        this.m_ElementsCM[3] = onecos_xy - zsin;
        this.m_ElementsCM[4] = cos + (oneminuscos * (axis.y * axis.y));
        this.m_ElementsCM[5] = onecos_yz + xsin;

        //Column 3
        this.m_ElementsCM[6] = onecos_xz + ysin;
        this.m_ElementsCM[7] = onecos_yz - xsin;
        this.m_ElementsCM[8] = cos + (oneminuscos * (axis.z * axis.z));
    }

    static ZeroMatrix(): Mat3 {
        let m = new Mat3();
        m.SetZero();
        return m;
    }

    static IdentityMatrix(): Mat3 {
        let m = new Mat3();
        return m;
    }

    Transpose(): void {
        let tmp: number;

        tmp = this.GetElement(0, 1);
        this.SetElement(0, 1, this.GetElement(1, 0));
        this.SetElement(1, 0, tmp);

        tmp = this.GetElement(0, 2);
        this.SetElement(0, 2, this.GetElement(2, 0));
        this.SetElement(2, 0, tmp);

        tmp = this.GetElement(1, 2);
        this.SetElement(1, 2, this.GetElement(2, 1));
        this.SetElement(2, 1, tmp);
    }

    GetTranspose(): Mat3 {
        let m = this.Clone();
        m.Transpose();
        return m;
    }

    SetRow(row: number, c1: number, c2: number, c3: number): void {
        this.m_ElementsCM[row] = c1;
        this.m_ElementsCM[3 + row] = c2;
        this.m_ElementsCM[6 + row] = c3;
    }

    SetColumn(column: number, r1: number, r2: number, r3: number): void {
        const off = column * 3;
        this.m_ElementsCM[off + 0] = r1;
        this.m_ElementsCM[off + 1] = r2;
        this.m_ElementsCM[off + 2] = r3;
    }

    SetDiagonal(d1: number, d2: number, d3: number, d4: number): void {
        this.m_ElementsCM[0] = d1;
        this.m_ElementsCM[4] = d2;
        this.m_ElementsCM[8] = d3;
    }

    Invert(epsilon: number = 0.00001): boolean {
        let inv = this.GetInverse(epsilon);

        if (inv == null)
            return false;

        this.SetMat3(inv);
        return true;
    }

    GetInverse(epsilon: number = 0.00001): Mat3 {
        let Inverse = new Mat3();

        const fDet =
            this.GetElement(0, 0) * (this.GetElement(2, 2) * this.GetElement(1, 1) - this.GetElement(1, 2) * this.GetElement(2, 1)) -
            this.GetElement(0, 1) * (this.GetElement(2, 2) * this.GetElement(1, 0) - this.GetElement(1, 2) * this.GetElement(2, 0)) +
            this.GetElement(0, 2) * (this.GetElement(2, 1) * this.GetElement(1, 0) - this.GetElement(1, 1) * this.GetElement(2, 0));

        if (ezMath.IsNumberZero(fDet, epsilon))
            return null;

        const fOneDivDet = 1 / fDet;

        Inverse.SetElement(0, 0, (this.GetElement(2, 2) * this.GetElement(1, 1) - this.GetElement(1, 2) * this.GetElement(2, 1)));
        Inverse.SetElement(0, 1, -(this.GetElement(2, 2) * this.GetElement(0, 1) - this.GetElement(0, 2) * this.GetElement(2, 1)));
        Inverse.SetElement(0, 2, (this.GetElement(1, 2) * this.GetElement(0, 1) - this.GetElement(0, 2) * this.GetElement(1, 1)));
        Inverse.SetElement(1, 0, -(this.GetElement(2, 2) * this.GetElement(1, 0) - this.GetElement(1, 2) * this.GetElement(2, 0)));
        Inverse.SetElement(1, 1, (this.GetElement(2, 2) * this.GetElement(0, 0) - this.GetElement(0, 2) * this.GetElement(2, 0)));
        Inverse.SetElement(1, 2, -(this.GetElement(1, 2) * this.GetElement(0, 0) - this.GetElement(0, 2) * this.GetElement(1, 0)));
        Inverse.SetElement(2, 0, (this.GetElement(2, 1) * this.GetElement(1, 0) - this.GetElement(1, 1) * this.GetElement(2, 0)));
        Inverse.SetElement(2, 1, -(this.GetElement(2, 1) * this.GetElement(0, 0) - this.GetElement(0, 1) * this.GetElement(2, 0)));
        Inverse.SetElement(2, 2, (this.GetElement(1, 1) * this.GetElement(0, 0) - this.GetElement(0, 1) * this.GetElement(1, 0)));

        return Inverse;
    }

    IsEqual(rhs: Mat3, epsilon: number): boolean {

        for (let i = 0; i < 9; ++i) {
            if (!ezMath.IsNumberEqual(this.m_ElementsCM[i], rhs.m_ElementsCM[i], epsilon)) {
                return false;
            }
        }

        return true;
    }

    IsZero(epsilon: number): boolean {
        for (let i = 0; i < 9; ++i) {
            if (!ezMath.IsNumberZero(this.m_ElementsCM[i], epsilon)) {
                return false;
            }
        }

        return true;
    }

    IsIdentity(epsilon: number): boolean {

        if (!ezMath.IsNumberEqual(this.m_ElementsCM[0], 1, epsilon))
            return false;
        if (!ezMath.IsNumberZero(this.m_ElementsCM[3], epsilon))
            return false;
        if (!ezMath.IsNumberZero(this.m_ElementsCM[6], epsilon))
            return false;

        if (!ezMath.IsNumberZero(this.m_ElementsCM[1], epsilon))
            return false;
        if (!ezMath.IsNumberEqual(this.m_ElementsCM[4], 1, epsilon))
            return false;
        if (!ezMath.IsNumberZero(this.m_ElementsCM[7], epsilon))
            return false;

        if (!ezMath.IsNumberZero(this.m_ElementsCM[2], epsilon))
            return false;
        if (!ezMath.IsNumberZero(this.m_ElementsCM[5], epsilon))
            return false;
        if (!ezMath.IsNumberEqual(this.m_ElementsCM[8], 1, epsilon))
            return false;

        return true;
    }

    IsIdentical(rhs: Mat3): boolean {
        for (let i = 0; i < 9; ++i) {
            if (this.m_ElementsCM[i] != rhs.m_ElementsCM[i]) {
                return false;
            }
        }

        return true;
    }

    GetScalingFactors(): Vec3 {
        let tmp = new Vec3();

        tmp.Set(this.GetElement(0, 0), this.GetElement(0, 1), this.GetElement(0, 2));
        const x = tmp.GetLength();

        tmp.Set(this.GetElement(1, 0), this.GetElement(1, 1), this.GetElement(1, 2));
        const y = tmp.GetLength();

        tmp.Set(this.GetElement(2, 0), this.GetElement(2, 1), this.GetElement(2, 2));
        const z = tmp.GetLength();

        tmp.Set(x, y, z);
        return tmp;
    }

    SetScalingFactors(x: number, y: number, z: number, epsilon: number): boolean {
        let tx = new Vec3(this.GetElement(0, 0), this.GetElement(0, 1), this.GetElement(0, 2));
        let ty = new Vec3(this.GetElement(1, 0), this.GetElement(1, 1), this.GetElement(1, 2));
        let tz = new Vec3(this.GetElement(2, 0), this.GetElement(2, 1), this.GetElement(2, 2));

        if (tx.SetLength(x, epsilon) == false)
            return false;
        if (ty.SetLength(y, epsilon) == false)
            return false;
        if (tz.SetLength(z, epsilon) == false)
            return false;

        this.SetElement(0, 0, tx.x);
        this.SetElement(0, 1, tx.y);
        this.SetElement(0, 2, tx.z);
        this.SetElement(1, 0, ty.x);
        this.SetElement(1, 1, ty.y);
        this.SetElement(1, 2, ty.z);
        this.SetElement(2, 0, tz.x);
        this.SetElement(2, 1, tz.y);
        this.SetElement(2, 2, tz.z);

        return true;
    }

    TransformDirection(dir: Vec3): void {
        const x = this.GetElement(0, 0) * dir.x + this.GetElement(1, 0) * dir.y + this.GetElement(2, 0) * dir.z;
        const y = this.GetElement(0, 1) * dir.x + this.GetElement(1, 1) * dir.y + this.GetElement(2, 1) * dir.z;
        const z = this.GetElement(0, 2) * dir.x + this.GetElement(1, 2) * dir.y + this.GetElement(2, 2) * dir.z;

        dir.Set(x, y, z);
    }

    MulNumber(factor: number): void {
        for (var i = 0; i < 9; ++i) {
            this.m_ElementsCM[i] *= factor;
        }
    }

    DivNumber(factor: number): void {
        const mul = 1 / factor;
        for (var i = 0; i < 9; ++i) {
            this.m_ElementsCM[i] *= mul;
        }
    }

    AddMat3(rhs: Mat3): void {
        for (var i = 0; i < 9; ++i) {
            this.m_ElementsCM[i] += rhs.m_ElementsCM[i];
        }
    }

    SubMat3(rhs: Mat3): void {
        for (var i = 0; i < 9; ++i) {
            this.m_ElementsCM[i] -= rhs.m_ElementsCM[i];
        }
    }

    SetMulMat3(lhs: Mat3, rhs: Mat3): void {

        for (let i = 0; i < 4; ++i) {
            this.SetElement(0, i, lhs.GetElement(0, i) * rhs.GetElement(0, 0) + lhs.GetElement(1, i) * rhs.GetElement(0, 1) + lhs.GetElement(2, i) * rhs.GetElement(0, 2) + lhs.GetElement(3, i) * rhs.GetElement(0, 3);
            this.SetElement(1, i, lhs.GetElement(0, i) * rhs.GetElement(1, 0) + lhs.GetElement(1, i) * rhs.GetElement(1, 1) + lhs.GetElement(2, i) * rhs.GetElement(1, 2) + lhs.GetElement(3, i) * rhs.GetElement(1, 3);
            this.SetElement(2, i, lhs.GetElement(0, i) * rhs.GetElement(2, 0) + lhs.GetElement(1, i) * rhs.GetElement(2, 1) + lhs.GetElement(2, i) * rhs.GetElement(2, 2) + lhs.GetElement(3, i) * rhs.GetElement(2, 3);
            this.SetElement(3, i, lhs.GetElement(0, i) * rhs.GetElement(3, 0) + lhs.GetElement(1, i) * rhs.GetElement(3, 1) + lhs.GetElement(2, i) * rhs.GetElement(3, 2) + lhs.GetElement(3, i) * rhs.GetElement(3, 3);
        }
    }

    GetRow(row: number): number[] {
        return [this.m_ElementsCM[row], this.m_ElementsCM[row + 3], this.m_ElementsCM[row + 6]];
    }

    GetColumn(column: number): number[] {
        return [this.m_ElementsCM[column * 3], this.m_ElementsCM[column * 3 + 1], this.m_ElementsCM[column * 3 + 2]];
    }

    GetDiagonal(): number[] {
        return [this.m_ElementsCM[0], this.m_ElementsCM[4], this.m_ElementsCM[8]];
    }

    SetFromArray(array: number[], isColumnMajor: boolean): void {

        if (isColumnMajor) {
            for (var i = 0; i < 9; ++i) {
                this.m_ElementsCM[i] = array[i];
            }
        }
        else {
            this.m_ElementsCM[0] = array[0];
            this.m_ElementsCM[1] = array[3];
            this.m_ElementsCM[2] = array[6];

            this.m_ElementsCM[3] = array[1];
            this.m_ElementsCM[4] = array[4];
            this.m_ElementsCM[5] = array[7];

            this.m_ElementsCM[6] = array[2];
            this.m_ElementsCM[7] = array[5];
            this.m_ElementsCM[8] = array[8];
        }
    }

    GetAsArray(asColumnMajor: boolean): number[] {

        if (asColumnMajor) {
            let array: number[] = [
                this.m_ElementsCM[0], this.m_ElementsCM[1], this.m_ElementsCM[2],
                this.m_ElementsCM[3], this.m_ElementsCM[4], this.m_ElementsCM[5],
                this.m_ElementsCM[6], this.m_ElementsCM[7], this.m_ElementsCM[8]];
            return array;
        }
        else {
            let array: number[] = [
                this.m_ElementsCM[0], this.m_ElementsCM[3], this.m_ElementsCM[6],
                this.m_ElementsCM[1], this.m_ElementsCM[4], this.m_ElementsCM[7],
                this.m_ElementsCM[2], this.m_ElementsCM[5], this.m_ElementsCM[8]];
            return array;
        }
    }
}