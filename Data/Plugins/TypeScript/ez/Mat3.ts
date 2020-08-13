import __Utils = require("./Utils")
export import Utils = __Utils.Utils;

import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

/**
 * A 3x3 matrix that can represent rotations and scaling, but no translation.
 */
export class Mat3 {
    m_ElementsCM: number[] = [
        1, 0, 0,
        0, 1, 0,
        0, 0, 1];

    /**
     * By default the constructor will initialize the matrix to identity.
     */
    constructor(c1r1: number = 1, c2r1: number = 0, c3r1: number = 0, c1r2: number = 0, c2r2: number = 1, c3r2: number = 0, c1r3: number = 0, c2r3: number = 0, c3r3: number = 1) { // [tested]
        this.SetElements(c1r1, c2r1, c3r1, c1r2, c2r2, c3r2, c1r3, c2r3, c3r3);
    }

    /**
     * Returns a duplicate of this matrix.
     */
    Clone(): Mat3 { // [tested]
        let c = new Mat3();
        c.SetMat3(this);
        return c;
    }

    /**
     * Copies the values of m into this.
     */
    SetMat3(m: Mat3): void { // [tested]
        for (var i = 0; i < 9; ++i) {
            this.m_ElementsCM[i] = m.m_ElementsCM[i];
        }
    }

    /**
     * Returns the value from the requested row and column.
     */
    GetElement(column: number, row: number): number { // [tested]
        return this.m_ElementsCM[column * 3 + row];
    }

    /**
     * Overwrites the value in the given row and column.
     */
    SetElement(column: number, row: number, value: number): void { // [tested]
        this.m_ElementsCM[column * 3 + row] = value;
    }

    /**
     * Sets all values of this matrix.
     */
    SetElements(c1r1: number, c2r1: number, c3r1: number, c1r2: number, c2r2: number, c3r2: number, c1r3: number, c2r3: number, c3r3: number): void { // [tested]
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

    /**
     * Sets all values to zero.
     */
    SetZero(): void { // [tested]
        for (var i = 0; i < 9; ++i) {
            this.m_ElementsCM[i] = 0;
        }
    }

    /**
     * Sets the matrix to be the identity matrix.
     */
    SetIdentity(): void { // [tested]
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

    /**
     * Sets this matrix to be a scaling matrix
     * @param scale How much the matrix scales along x, y and z.
     */
    SetScalingMatrix(scale: Vec3): void { // [tested]
        this.SetElements(
            scale.x, 0, 0,
            0, scale.y, 0,
            0, 0, scale.z);
    }

    /**
     * Sets the matrix to rotate objects around the X axis.
     * 
     * @param radians The angle of rotation in radians.
     */
    SetRotationMatrixX(radians: number): void { // [tested]
        const fSin = Math.sin(radians);
        const fCos = Math.cos(radians);

        this.SetElements(
            1, 0, 0,
            0, fCos, -fSin,
            0, fSin, fCos);
    }

    /**
     * Sets the matrix to rotate objects around the Y axis.
     * 
     * @param radians The angle of rotation in radians.
     */
    SetRotationMatrixY(radians: number): void { // [tested]
        const fSin = Math.sin(radians);
        const fCos = Math.cos(radians);

        this.SetElements(
            fCos, 0, fSin,
            0, 1, 0,
            -fSin, 0, fCos);
    }

    /**
     * Sets the matrix to rotate objects around the Z axis.
     * 
     * @param radians The angle of rotation in radians.
     */
    SetRotationMatrixZ(radians: number): void { // [tested]
        const fSin = Math.sin(radians);
        const fCos = Math.cos(radians);

        this.SetElements(
            fCos, -fSin, 0,
            fSin, fCos, 0,
            0, 0, 1);
    }

    /**
     * Sets the matrix to rotate objects around an arbitrary axis.
     * 
     * @param axis The normalized axis around which to rotate.
     * @param radians The angle of rotation in radians.
     */
    SetRotationMatrix(axis: Vec3, radians: number): void { // [tested]

        const cos = Math.cos(radians);
        const sin = Math.sin(radians);
        const oneMinusCos = 1 - cos;

        const xy = axis.x * axis.y;
        const xz = axis.x * axis.z;
        const yz = axis.y * axis.z;

        const xSin = axis.x * sin;
        const ySin = axis.y * sin;
        const zSin = axis.z * sin;

        const oneCos_xy = oneMinusCos * xy;
        const oneCos_xz = oneMinusCos * xz;
        const oneCos_yz = oneMinusCos * yz;

        //Column 1
        this.m_ElementsCM[0] = cos + (oneMinusCos * (axis.x * axis.x));
        this.m_ElementsCM[1] = oneCos_xy + zSin;
        this.m_ElementsCM[2] = oneCos_xz - ySin;

        //Column 2
        this.m_ElementsCM[3] = oneCos_xy - zSin;
        this.m_ElementsCM[4] = cos + (oneMinusCos * (axis.y * axis.y));
        this.m_ElementsCM[5] = oneCos_yz + xSin;

        //Column 3
        this.m_ElementsCM[6] = oneCos_xz + ySin;
        this.m_ElementsCM[7] = oneCos_yz - xSin;
        this.m_ElementsCM[8] = cos + (oneMinusCos * (axis.z * axis.z));
    }

    /**
     * Returns an all-zero matrix.
     */
    static ZeroMatrix(): Mat3 { // [tested]
        let m = new Mat3();
        m.SetZero();
        return m;
    }

    /**
     * Returns an identity matrix.
     */
    static IdentityMatrix(): Mat3 { // [tested]
        let m = new Mat3();
        return m;
    }

    /**
     * Flips all values along the diagonal.
     */
    Transpose(): void { // [tested]
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

    /**
     * Returns a transposed clone of this matrix.
     */
    GetTranspose(): Mat3 { // [tested]
        let m = this.Clone();
        m.Transpose();
        return m;
    }

    /**
     * Sets the values in the given row.
     */
    SetRow(row: number, c1: number, c2: number, c3: number): void { // [tested]
        this.m_ElementsCM[row] = c1;
        this.m_ElementsCM[3 + row] = c2;
        this.m_ElementsCM[6 + row] = c3;
    }

    /**
     * Sets the values in the given column.
     */
    SetColumn(column: number, r1: number, r2: number, r3: number): void { // [tested]
        const off = column * 3;
        this.m_ElementsCM[off + 0] = r1;
        this.m_ElementsCM[off + 1] = r2;
        this.m_ElementsCM[off + 2] = r3;
    }

    /**
     * Sets the values on the diagonal.
     */
    SetDiagonal(d1: number, d2: number, d3: number): void { // [tested]
        this.m_ElementsCM[0] = d1;
        this.m_ElementsCM[4] = d2;
        this.m_ElementsCM[8] = d3;
    }

    /**
     * Inverts this matrix, if possible.
     * 
     * @param epsilon The epsilon to determine whether this matrix can be inverted at all.
     * @returns true if the matrix could be inverted, false if inversion failed. In case of failure, this is unchanged.
     */
    Invert(epsilon: number = 0.00001): boolean { // [tested]
        let inv = this.GetInverse(epsilon);

        if (inv == null)
            return false;

        this.SetMat3(inv);
        return true;
    }

    /**
     * Returns an inverted clone of this or null if inversion failed.
     * 
     * @param epsilon The epsilon to determine whether this matrix can be inverted at all.
     */
    GetInverse(epsilon: number = 0.00001): Mat3 { // [tested]
        let Inverse = new Mat3();

        const fDet =
            this.GetElement(0, 0) * (this.GetElement(2, 2) * this.GetElement(1, 1) - this.GetElement(1, 2) * this.GetElement(2, 1)) -
            this.GetElement(0, 1) * (this.GetElement(2, 2) * this.GetElement(1, 0) - this.GetElement(1, 2) * this.GetElement(2, 0)) +
            this.GetElement(0, 2) * (this.GetElement(2, 1) * this.GetElement(1, 0) - this.GetElement(1, 1) * this.GetElement(2, 0));

        if (Utils.IsNumberZero(fDet, epsilon))
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

        Inverse.MulNumber(fOneDivDet);

        return Inverse;
    }

    /**
     * Checks whether this and rhs have equal values within a certain epsilon.
     */
    IsEqual(rhs: Mat3, epsilon: number): boolean { // [tested]

        for (let i = 0; i < 9; ++i) {
            if (!Utils.IsNumberEqual(this.m_ElementsCM[i], rhs.m_ElementsCM[i], epsilon)) {
                return false;
            }
        }

        return true;
    }

    /**
     * Checks whether this has all zero values within a certain epsilon.
     */
    IsZero(epsilon: number = 0.0001): boolean { // [tested]
        for (let i = 0; i < 9; ++i) {
            if (!Utils.IsNumberZero(this.m_ElementsCM[i], epsilon)) {
                return false;
            }
        }

        return true;
    }

    /**
     * Checks whether this is an identity matrix within a certain epsilon.
     */
    IsIdentity(epsilon: number = 0.0001): boolean { // [tested]

        if (!Utils.IsNumberEqual(this.m_ElementsCM[0], 1, epsilon))
            return false;
        if (!Utils.IsNumberZero(this.m_ElementsCM[3], epsilon))
            return false;
        if (!Utils.IsNumberZero(this.m_ElementsCM[6], epsilon))
            return false;

        if (!Utils.IsNumberZero(this.m_ElementsCM[1], epsilon))
            return false;
        if (!Utils.IsNumberEqual(this.m_ElementsCM[4], 1, epsilon))
            return false;
        if (!Utils.IsNumberZero(this.m_ElementsCM[7], epsilon))
            return false;

        if (!Utils.IsNumberZero(this.m_ElementsCM[2], epsilon))
            return false;
        if (!Utils.IsNumberZero(this.m_ElementsCM[5], epsilon))
            return false;
        if (!Utils.IsNumberEqual(this.m_ElementsCM[8], 1, epsilon))
            return false;

        return true;
    }

    /**
     * Checks whether this and rhs are completely identical without any epsilon comparison.
     */
    IsIdentical(rhs: Mat3): boolean { // [tested]
        for (let i = 0; i < 9; ++i) {
            if (this.m_ElementsCM[i] != rhs.m_ElementsCM[i]) {
                return false;
            }
        }

        return true;
    }

    /**
     * Tries to extract the scaling factors for x, y and z within this matrix.
     */
    GetScalingFactors(): Vec3 { // [tested]
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

    /**
     * Tries to rescale this matrix such that it applies the given scaling.
     * 
     * @param epsilon The epsilon to detect whether rescaling the matrix is possible.
     * @returns True if successful, false if the desired scaling could not be baked into the matrix.
     */
    SetScalingFactors(x: number, y: number, z: number, epsilon: number = 0.0001): boolean { // [tested]
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

    /**
     * Modifies the incoming vector by multiplying this from the left.
     *   dir = this * dir
     */
    TransformDirection(dir: Vec3): void { // [tested]
        const x = this.GetElement(0, 0) * dir.x + this.GetElement(1, 0) * dir.y + this.GetElement(2, 0) * dir.z;
        const y = this.GetElement(0, 1) * dir.x + this.GetElement(1, 1) * dir.y + this.GetElement(2, 1) * dir.z;
        const z = this.GetElement(0, 2) * dir.x + this.GetElement(1, 2) * dir.y + this.GetElement(2, 2) * dir.z;

        dir.Set(x, y, z);
    }

    /**
     * Multiplies all values in this matrix with 'factor'.
     */
    MulNumber(factor: number): void { // [tested]
        for (var i = 0; i < 9; ++i) {
            this.m_ElementsCM[i] *= factor;
        }
    }

    /**
     * Divides all values in this matrix by 'factor'.
     */
    DivNumber(factor: number): void { // [tested]
        const mul = 1 / factor;
        for (var i = 0; i < 9; ++i) {
            this.m_ElementsCM[i] *= mul;
        }
    }

    /**
     * Adds the components of rhs into the components of this.
     */
    AddMat3(rhs: Mat3): void { // [tested]
        for (var i = 0; i < 9; ++i) {
            this.m_ElementsCM[i] += rhs.m_ElementsCM[i];
        }
    }

    /**
     * Subtracts the components of rhs from the components of this.
     */
    SubMat3(rhs: Mat3): void { // [tested]
        for (var i = 0; i < 9; ++i) {
            this.m_ElementsCM[i] -= rhs.m_ElementsCM[i];
        }
    }

    /**
     * Sets this matrix to be the product of lhs and rhs.
     * 
     *   this = lhs * rhs
     */
    SetMulMat3(lhs: Mat3, rhs: Mat3): void { // [tested]

        for (let i = 0; i < 3; ++i) {
            this.SetElement(0, i, lhs.GetElement(0, i) * rhs.GetElement(0, 0) + lhs.GetElement(1, i) * rhs.GetElement(0, 1) + lhs.GetElement(2, i) * rhs.GetElement(0, 2));
            this.SetElement(1, i, lhs.GetElement(0, i) * rhs.GetElement(1, 0) + lhs.GetElement(1, i) * rhs.GetElement(1, 1) + lhs.GetElement(2, i) * rhs.GetElement(1, 2));
            this.SetElement(2, i, lhs.GetElement(0, i) * rhs.GetElement(2, 0) + lhs.GetElement(1, i) * rhs.GetElement(2, 1) + lhs.GetElement(2, i) * rhs.GetElement(2, 2));
        }
    }

    /**
     * Returns the values in 'row' as a 3-element array.
     */
    GetRow(row: number): number[] { // [tested]
        return [this.m_ElementsCM[row], this.m_ElementsCM[row + 3], this.m_ElementsCM[row + 6]];
    }

    /**
     * Returns the values in 'column' as a 3-element array.
     */
    GetColumn(column: number): number[] { // [tested]
        return [this.m_ElementsCM[column * 3], this.m_ElementsCM[column * 3 + 1], this.m_ElementsCM[column * 3 + 2]];
    }

    /**
     * Returns the values in from the diagonal as a 3-element array.
     */
    GetDiagonal(): number[] { // [tested]
        return [this.m_ElementsCM[0], this.m_ElementsCM[4], this.m_ElementsCM[8]];
    }

    /**
     * Sets all elements in this by copying them from an array.
     * 
     * @param array The array with the 9 values to copy.
     * @param isColumnMajor Whether the data in the array is column-major or row-major.
     */
    SetFromArray(array: number[], isColumnMajor: boolean): void { // [tested]

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

    /**
     * Returns the values of the matrix as an array.
     * 
     * @param asColumnMajor Whether the array should contain the values in column-major or row-major order.
     */
    GetAsArray(asColumnMajor: boolean): number[] { // [tested]

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