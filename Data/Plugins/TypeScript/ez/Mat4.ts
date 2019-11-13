import __Math = require("./Math")
export import ezMath = __Math.ezMath;

import __radians = require("./radians")
export import radians = __radians.radians;

import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

export class Mat4 {
    m_ElementsCM: number[]
        = [1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1];

    Clone(): Mat4 {
        let c = new Mat4;
        c.SetMat4(this);
        return c;
    }

    //CloneAsVec3() : Mat3{ }

    SetMat4(m: Mat4): void {
        for (var i = 0; i < 16; ++i) {
            this.m_ElementsCM[i] = m[i];
        }
    }

    GetElement(column: number, row: number): number {
        return this.m_ElementsCM[column * 4 + row];
    }

    SetElement(column: number, row: number, value: number): void {
        this.m_ElementsCM[column * 4 + row] = value;
    }

    SetElements(c1r1: number, c2r1: number, c3r1: number, c4r1: number, c1r2: number, c2r2: number, c3r2: number, c4r2: number, c1r3: number, c2r3: number, c3r3: number, c4r3: number, c1r4: number, c2r4: number, c3r4: number, c4r4: number): void {
        this.m_ElementsCM[0] = c1r1;
        this.m_ElementsCM[4] = c2r1;
        this.m_ElementsCM[8] = c3r1;
        this.m_ElementsCM[12] = c4r1;

        this.m_ElementsCM[1] = c1r2;
        this.m_ElementsCM[5] = c2r2;
        this.m_ElementsCM[9] = c3r2;
        this.m_ElementsCM[13] = c4r2;

        this.m_ElementsCM[2] = c1r3;
        this.m_ElementsCM[6] = c2r3;
        this.m_ElementsCM[10] = c3r3;
        this.m_ElementsCM[14] = c4r3;

        this.m_ElementsCM[3] = c1r4;
        this.m_ElementsCM[7] = c2r4;
        this.m_ElementsCM[11] = c3r4;
        this.m_ElementsCM[15] = c4r4;
    }

    SetZero(): void {
        for (var i = 0; i < 16; ++i) {
            this.m_ElementsCM[i] = 0;
        }
    }

    SetIdentity(): void {
        this.m_ElementsCM[0] = 1;
        this.m_ElementsCM[4] = 0;
        this.m_ElementsCM[8] = 0;
        this.m_ElementsCM[12] = 0;

        this.m_ElementsCM[1] = 0;
        this.m_ElementsCM[5] = 1;
        this.m_ElementsCM[9] = 0;
        this.m_ElementsCM[13] = 0;

        this.m_ElementsCM[2] = 0;
        this.m_ElementsCM[6] = 0;
        this.m_ElementsCM[10] = 1;
        this.m_ElementsCM[14] = 0;

        this.m_ElementsCM[3] = 0;
        this.m_ElementsCM[7] = 0;
        this.m_ElementsCM[11] = 0;
        this.m_ElementsCM[15] = 1;
    }

    SetTranslationMatrix(translation: Vec3): void {
        this.SetElements(1, 0, 0, translation.x,
            0, 1, 0, translation.y,
            0, 0, 1, translation.z,
            0, 0, 0, 1);
    }

    SetScalingMatrix(scale: Vec3): void {
        this.SetElements(scale.x, 0, 0, 0,
            0, scale.y, 0, 0,
            0, 0, scale.z, 0,
            0, 0, 0, 1);
    }

    SetRotationMatrixX(radians: number): void {
        const fSin = Math.sin(radians);
        const fCos = Math.cos(radians);

        this.SetElements(1, 0, 0, 0,
            0, fCos, -fSin, 0,
            0, fSin, fCos, 0,
            0, 0, 0, 1);
    }

    SetRotationMatrixY(radians: number): void {
        const fSin = Math.sin(radians);
        const fCos = Math.cos(radians);


        this.SetElements(fCos, 0, fSin, 0,
            0, 1, 0, 0,
            -fSin, 0, fCos, 0,
            0, 0, 0, 1);
    }

    SetRotationMatrixZ(radians: number): void {
        const fSin = Math.sin(radians);
        const fCos = Math.cos(radians);

        this.SetElements(fCos, -fSin, 0, 0,
            fSin, fCos, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1);
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
        this.m_ElementsCM[3] = 0;

        //Column 2
        this.m_ElementsCM[4] = onecos_xy - zsin;
        this.m_ElementsCM[5] = cos + (oneminuscos * (axis.y * axis.y));
        this.m_ElementsCM[6] = onecos_yz + xsin;
        this.m_ElementsCM[7] = 0;

        //Column 3
        this.m_ElementsCM[8] = onecos_xz + ysin;
        this.m_ElementsCM[9] = onecos_yz - xsin;
        this.m_ElementsCM[10] = cos + (oneminuscos * (axis.z * axis.z));
        this.m_ElementsCM[11] = 0;

        //Column 4
        this.m_ElementsCM[12] = 0;
        this.m_ElementsCM[13] = 0;
        this.m_ElementsCM[14] = 0;
        this.m_ElementsCM[15] = 1;
    }

    static ZeroMatrix(): Mat4 {
        let m = new Mat4();
        m.SetZero();
        return m;
    }

    static IdentityMatrix(): Mat4 {
        let m = new Mat4();
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

        tmp = this.GetElement(0, 3);
        this.SetElement(0, 3, this.GetElement(3, 0));
        this.SetElement(3, 0, tmp);

        tmp = this.GetElement(1, 2);
        this.SetElement(1, 2, this.GetElement(2, 1));
        this.SetElement(2, 1, tmp);

        tmp = this.GetElement(1, 3);
        this.SetElement(1, 3, this.GetElement(3, 1));
        this.SetElement(3, 1, tmp);

        tmp = this.GetElement(2, 3);
        this.SetElement(2, 3, this.GetElement(3, 2));
        this.SetElement(3, 2, tmp);
    }

    GetTranspose(): Mat4 {
        let m = this.Clone();
        m.Transpose();
        return m;
    }

    SetRow(row: number, c1: number, c2: number, c3: number, c4: number): void {
        this.m_ElementsCM[row] = c1;
        this.m_ElementsCM[4 + row] = c2;
        this.m_ElementsCM[8 + row] = c3;
        this.m_ElementsCM[12 + row] = c4;
    }

    SetColumn(column: number, r1: number, r2: number, r3: number, r4: number): void {
        const off = column * 4;
        this.m_ElementsCM[off + 0] = r1;
        this.m_ElementsCM[off + 1] = r2;
        this.m_ElementsCM[off + 2] = r3;
        this.m_ElementsCM[off + 3] = r4;
    }

    SetDiagonal(d1: number, d2: number, d3: number, d4: number): void {
        this.m_ElementsCM[0] = d1;
        this.m_ElementsCM[5] = d2;
        this.m_ElementsCM[10] = d3;
        this.m_ElementsCM[15] = d4;
    }

    Invert(epsilon: number = 0.00001): boolean {
        let inv = this.GetInverse(epsilon);

        if (inv == null)
            return false;

        this.SetMat4(inv);
        return true;
    }

    GetInverse(epsilon: number = 0.00001): Mat4 {
        let Inverse = new Mat4();

        const fDet = this.GetDeterminantOf4x4Matrix();

        if (ezMath.IsNumberEqual(fDet, 0, epsilon))
            return null;

        let fOneDivDet = 1.0 / fDet;

        for (let i = 0; i < 4; ++i) {

            Inverse.SetElement(i, 0, this.GetDeterminantOf3x3SubMatrix(i, 0) * fOneDivDet);
            fOneDivDet = -fOneDivDet;
            Inverse.SetElement(i, 1, this.GetDeterminantOf3x3SubMatrix(i, 1) * fOneDivDet);
            fOneDivDet = -fOneDivDet;
            Inverse.SetElement(i, 2, this.GetDeterminantOf3x3SubMatrix(i, 2) * fOneDivDet);
            fOneDivDet = -fOneDivDet;
            Inverse.SetElement(i, 3, this.GetDeterminantOf3x3SubMatrix(i, 3) * fOneDivDet);
        }

        return Inverse;
    }

    IsEqual(rhs: Mat4, epsilon: number): boolean {

        for (let i = 0; i < 16; ++i) {
            if (!ezMath.IsNumberEqual(this.m_ElementsCM[i], rhs.m_ElementsCM[i], epsilon)) {
                return false;
            }
        }

        return true;
    }

    IsZero(epsilon: number): boolean {
        for (let i = 0; i < 16; ++i) {
            if (!ezMath.IsNumberZero(this.m_ElementsCM[i], epsilon)) {
                return false;
            }
        }

        return true;
    }

    IsIdentity(epsilon: number): boolean {

        if (!ezMath.IsNumberEqual(this.m_ElementsCM[0], 1, epsilon))
            return false;
        if (!ezMath.IsNumberZero(this.m_ElementsCM[4], epsilon))
            return false;
        if (!ezMath.IsNumberZero(this.m_ElementsCM[8], epsilon))
            return false;
        if (!ezMath.IsNumberZero(this.m_ElementsCM[12], epsilon))
            return false;


        if (!ezMath.IsNumberZero(this.m_ElementsCM[1], epsilon))
            return false;
        if (!ezMath.IsNumberEqual(this.m_ElementsCM[5], 1, epsilon))
            return false;
        if (!ezMath.IsNumberZero(this.m_ElementsCM[9], epsilon))
            return false;
        if (!ezMath.IsNumberZero(this.m_ElementsCM[13], epsilon))
            return false;

        if (!ezMath.IsNumberZero(this.m_ElementsCM[2], epsilon))
            return false;
        if (!ezMath.IsNumberZero(this.m_ElementsCM[6], epsilon))
            return false;
        if (!ezMath.IsNumberEqual(this.m_ElementsCM[10], 1, epsilon))
            return false;
        if (!ezMath.IsNumberZero(this.m_ElementsCM[14], epsilon))
            return false;

        if (!ezMath.IsNumberZero(this.m_ElementsCM[3], epsilon))
            return false;
        if (!ezMath.IsNumberZero(this.m_ElementsCM[7], epsilon))
            return false;
        if (!ezMath.IsNumberZero(this.m_ElementsCM[11], epsilon))
            return false;
        if (!ezMath.IsNumberEqual(this.m_ElementsCM[15], 1, epsilon))
            return false;

        return true;
    }

    IsIdentical(rhs: Mat4): boolean {
        for (let i = 0; i < 16; ++i) {
            if (this.m_ElementsCM[i] != rhs.m_ElementsCM[i]) {
                return false;
            }
        }

        return true;
    }

    GetTranslationVector(): Vec3 {
        return new Vec3(this.m_ElementsCM[12], this.m_ElementsCM[13], this.m_ElementsCM[14]);
    }

    SetTranslationVector(translation: Vec3): void {
        this.m_ElementsCM[12] = translation.x;
        this.m_ElementsCM[13] = translation.y;
        this.m_ElementsCM[14] = translation.z;
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

    TransformPosition(pos: Vec3): void {
        const x = this.GetElement(0, 0) * pos.x + this.GetElement(1, 0) * pos.y + this.GetElement(2, 0) * pos.z + this.GetElement(3, 0);
        const y = this.GetElement(0, 1) * pos.x + this.GetElement(1, 1) * pos.y + this.GetElement(2, 1) * pos.z + this.GetElement(3, 1);
        const z = this.GetElement(0, 2) * pos.x + this.GetElement(1, 2) * pos.y + this.GetElement(2, 2) * pos.z + this.GetElement(3, 2);

        pos.Set(x, y, z);
    }

    TransformDirection(dir: Vec3): void {
        const x = this.GetElement(0, 0) * dir.x + this.GetElement(1, 0) * dir.y + this.GetElement(2, 0) * dir.z;
        const y = this.GetElement(0, 1) * dir.x + this.GetElement(1, 1) * dir.y + this.GetElement(2, 1) * dir.z;
        const z = this.GetElement(0, 2) * dir.x + this.GetElement(1, 2) * dir.y + this.GetElement(2, 2) * dir.z;

        dir.Set(x, y, z);
    }

    MulNumber(factor: number): void {
        for (var i = 0; i < 16; ++i) {
            this.m_ElementsCM[i] *= factor;
        }
    }

    DivNumber(factor: number): void {
        const mul = 1 / factor;
        for (var i = 0; i < 16; ++i) {
            this.m_ElementsCM[i] *= mul;
        }
    }

    AddMat4(rhs: Mat4): void {
        for (var i = 0; i < 16; ++i) {
            this.m_ElementsCM[i] += rhs.m_ElementsCM[i];
        }
    }

    SubMat4(rhs: Mat4): void {
        for (var i = 0; i < 16; ++i) {
            this.m_ElementsCM[i] -= rhs.m_ElementsCM[i];
        }
    }

    SetMulMat4(lhs: Mat4, rhs: Mat4): void {

        for (let i = 0; i < 4; ++i) {
            this.SetElement(0, i, lhs.GetElement(0, i) * rhs.GetElement(0, 0) + lhs.GetElement(1, i) * rhs.GetElement(0, 1) + lhs.GetElement(2, i) * rhs.GetElement(0, 2) + lhs.GetElement(3, i) * rhs.GetElement(0, 3);
            this.SetElement(1, i, lhs.GetElement(0, i) * rhs.GetElement(1, 0) + lhs.GetElement(1, i) * rhs.GetElement(1, 1) + lhs.GetElement(2, i) * rhs.GetElement(1, 2) + lhs.GetElement(3, i) * rhs.GetElement(1, 3);
            this.SetElement(2, i, lhs.GetElement(0, i) * rhs.GetElement(2, 0) + lhs.GetElement(1, i) * rhs.GetElement(2, 1) + lhs.GetElement(2, i) * rhs.GetElement(2, 2) + lhs.GetElement(3, i) * rhs.GetElement(2, 3);
            this.SetElement(3, i, lhs.GetElement(0, i) * rhs.GetElement(3, 0) + lhs.GetElement(1, i) * rhs.GetElement(3, 1) + lhs.GetElement(2, i) * rhs.GetElement(3, 2) + lhs.GetElement(3, i) * rhs.GetElement(3, 3);
        }
    }

    // ezVec4Template<Type> GetRow(ezUInt32 uiRow) const;
    // ezVec4Template<Type> GetColumn(ezUInt32 uiColumn) const;
    // ezVec4Template<Type> GetDiagonal() const;
    // void SetFromArray(const Type* const pData, ezMatrixLayout::Enum layout);
    // void GetAsArray(Type* out_pData, ezMatrixLayout::Enum layout) const;
    // void SetTransformationMatrix(const ezMat3Template<Type>& Rotation, const ezVec3Template<Type>& translation);
    // void SetRotationalPart(const ezMat3Template<Type>& Rotation);
    // const ezMat3Template<Type> GetRotationalPart() const;

    private GetDeterminantOf3x3SubMatrix(i: number, j: number): number {
        const si0 = 0 + ((i <= 0) ? 1 : 0);
        const si1 = 1 + ((i <= 1) ? 1 : 0);
        const si2 = 2 + ((i <= 2) ? 1 : 0);

        const sj0 = 0 + ((j <= 0) ? 1 : 0);
        const sj1 = 1 + ((j <= 1) ? 1 : 0);
        const sj2 = 2 + ((j <= 2) ? 1 : 0);

        const fDet2 = ((this.GetElement(sj0, si0) * this.GetElement(sj1, si1) * this.GetElement(sj2, si2) +
            this.GetElement(sj1, si0) * this.GetElement(sj2, si1) * this.GetElement(sj0, si2) +
            this.GetElement(sj2, si0) * this.GetElement(sj0, si1) * this.GetElement(sj1, si2)) -
            (this.GetElement(sj0, si2) * this.GetElement(sj1, si1) * this.GetElement(sj2, si0) +
                this.GetElement(sj1, si2) * this.GetElement(sj2, si1) * this.GetElement(sj0, si0) +
                this.GetElement(sj2, si2) * this.GetElement(sj0, si1) * this.GetElement(sj1, si0)));

        return fDet2;
    }

    private GetDeterminantOf4x4Matrix(): number {
        let det = 0.0;

        det += this.GetElement(0, 0) * this.GetDeterminantOf3x3SubMatrix(0, 0);
        det += -this.GetElement(1, 0) * this.GetDeterminantOf3x3SubMatrix(0, 1);
        det += this.GetElement(2, 0) * this.GetDeterminantOf3x3SubMatrix(0, 2);
        det += -this.GetElement(3, 0) * this.GetDeterminantOf3x3SubMatrix(0, 3);

        return det;
    }
}