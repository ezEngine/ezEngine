import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

import __Angle = require("./Angle")
export import Angle = __Angle.Angle;

import __Mat3 = require("./Mat3")
export import Mat3 = __Mat3.Mat3;

import __Mat4 = require("./Mat4")
export import Mat4 = __Mat4.Mat4;

export import Utils = __Vec3.Utils;

export class Quat {
    x: number;
    y: number;
    z: number;
    w: number;

    constructor(_x: number = 0.0, _y: number = 0.0, _z: number = 0.0, _w: number = 1.0) {
        this.x = _x;
        this.y = _y;
        this.z = _z;
        this.w = _w;
    }

    Clone(): Quat {
        return new Quat(this.x, this.y, this.z, this.w);
    }

    GetAsMat3(): Mat3 {
        let m = new Mat3();

        const fTx = this.x + this.x;
        const fTy = this.y + this.y;
        const fTz = this.z + this.z;
        const fTwx = fTx * this.w;
        const fTwy = fTy * this.w;
        const fTwz = fTz * this.w;
        const fTxx = fTx * this.x;
        const fTxy = fTy * this.x;
        const fTxz = fTz * this.x;
        const fTyy = fTy * this.y;
        const fTyz = fTz * this.y;
        const fTzz = fTz * this.z;

        m.SetElement(0, 0, 1 - (fTyy + fTzz));
        m.SetElement(1, 0, fTxy - fTwz);
        m.SetElement(2, 0, fTxz + fTwy);
        m.SetElement(0, 1, fTxy + fTwz);
        m.SetElement(1, 1, 1 - (fTxx + fTzz));
        m.SetElement(2, 1, fTyz - fTwx);
        m.SetElement(0, 2, fTxz - fTwy);
        m.SetElement(1, 2, fTyz + fTwx);
        m.SetElement(2, 2, 1 - (fTxx + fTyy));

        return m;
    }

    GetAsMat4(): Mat4 {
        let m = new Mat4();

        const fTx = this.x + this.x;
        const fTy = this.y + this.y;
        const fTz = this.z + this.z;
        const fTwx = fTx * this.w;
        const fTwy = fTy * this.w;
        const fTwz = fTz * this.w;
        const fTxx = fTx * this.x;
        const fTxy = fTy * this.x;
        const fTxz = fTz * this.x;
        const fTyy = fTy * this.y;
        const fTyz = fTz * this.y;
        const fTzz = fTz * this.z;

        m.SetElement(0, 0, 1 - (fTyy + fTzz));
        m.SetElement(1, 0, fTxy - fTwz);
        m.SetElement(2, 0, fTxz + fTwy);
        m.SetElement(0, 1, fTxy + fTwz);
        m.SetElement(1, 1, 1 - (fTxx + fTzz));
        m.SetElement(2, 1, fTyz - fTwx);
        m.SetElement(0, 2, fTxz - fTwy);
        m.SetElement(1, 2, fTyz + fTwx);
        m.SetElement(2, 2, 1 - (fTxx + fTyy));

        return m;
    }

    SetFromMat3(m: Mat3): void {
        const trace = m.GetElement(0, 0) + m.GetElement(1, 1) + m.GetElement(2, 2);
        const half = 0.5;

        let val: number[] = [0, 0, 0, 0];

        if (trace > 0) {
            let s = Math.sqrt(trace + 1);
            let t = half / s;

            val[0] = (m.GetElement(1, 2) - m.GetElement(2, 1)) * t;
            val[1] = (m.GetElement(2, 0) - m.GetElement(0, 2)) * t;
            val[2] = (m.GetElement(0, 1) - m.GetElement(1, 0)) * t;

            val[3] = half * s;
        }
        else {
            const next: number[] = [1, 2, 0];
            let i = 0;

            if (m.GetElement(1, 1) > m.GetElement(0, 0))
                i = 1;

            if (m.GetElement(2, 2) > m.GetElement(i, i))
                i = 2;

            let j = next[i];
            let k = next[j];

            let s = Math.sqrt(m.GetElement(i, i) - (m.GetElement(j, j) + m.GetElement(k, k)) + 1);
            let t = half / s;

            val[i] = half * s;
            val[3] = (m.GetElement(j, k) - m.GetElement(k, j)) * t;
            val[j] = (m.GetElement(i, j) + m.GetElement(j, i)) * t;
            val[k] = (m.GetElement(i, k) + m.GetElement(k, i)) * t;
        }

        this.x = val[0];
        this.y = val[1];
        this.z = val[2];
        this.w = val[3];
    }

    SetQuat(rhs: Quat): void {
        this.x = rhs.x;
        this.y = rhs.y;
        this.z = rhs.z;
        this.w = rhs.w;
    }

    SetIdentity(): void {
        this.x = 0;
        this.y = 0;
        this.z = 0;
        this.w = 1.0;
    }

    static IdentityQuaternion(): Quat {
        return new Quat();
    }

    Normalize(): void {
        let n = this.x * this.x + this.y * this.y + this.z * this.z + this.w * this.w;

        n = 1.0 / Math.sqrt(n);

        this.x *= n;
        this.y *= n;
        this.z *= n;
        this.w *= n;
    }

    Negate(): void {
        this.x = -this.x;
        this.y = -this.y;
        this.z = -this.z;
    }

    GetNegated(): Quat {
        return new Quat(-this.x, -this.y, -this.z, this.w);
    }

    SetFromAxisAndAngle(vRotationAxis: Vec3, angleInRadian: number) {
        const halfAngle = angleInRadian * 0.5;

        let sinHalfAngle = Math.sin(halfAngle);

        this.x = vRotationAxis.x * sinHalfAngle;
        this.y = vRotationAxis.y * sinHalfAngle;
        this.z = vRotationAxis.z * sinHalfAngle;
        this.w = Math.cos(halfAngle);
    }

    SetShortestRotation(vDirFrom: Vec3, vDirTo: Vec3): void {
        const v0 = vDirFrom.GetNormalized();
        const v1 = vDirTo.GetNormalized();

        const fDot = v0.Dot(v1);

        // if both vectors are identical -> no rotation needed
        if (Utils.IsNumberEqual(fDot, 1.0, 0.0001)) {
            this.SetIdentity();
            return;
        }
        else if (Utils.IsNumberEqual(fDot, -1.0, 0.0001)) // if both vectors are opposing
        {
            // find an axis, that is not identical and not opposing, ezVec3Template::Cross-product to find perpendicular vector, rotate around that

            if (Math.abs(v0.x) < 0.8)
                this.SetFromAxisAndAngle(v0.CrossRH(new Vec3(1, 0, 0)).GetNormalized(), Math.PI);
            else
                this.SetFromAxisAndAngle(v0.CrossRH(new Vec3(0, 1, 0)).GetNormalized(), Math.PI);

            return;
        }

        const c = v0.CrossRH(v1);
        const d = v0.Dot(v1);
        const s = Math.sqrt((1.0 + d) * 2.0);
        const invS = 1.0 / s;

        this.x = c.x * invS;
        this.y = c.y * invS;
        this.z = c.z * invS;
        this.w = s * 0.5;

        this.Normalize();
    }

    SetSlerp(qFrom: Quat, qTo: Quat, t: number): void {
        const qdelta = 0.009;

        let cosTheta = (qFrom.x * qTo.x + qFrom.y * qTo.y + qFrom.z * qTo.z + qFrom.w * qTo.w);

        let bFlipSign = false;

        if (cosTheta < 0.0) {
            bFlipSign = true;
            cosTheta = -cosTheta;
        }

        let t0: number, t1: number;

        if (cosTheta < qdelta) {
            let theta = Math.acos(cosTheta);

            // use sqrtInv(1+c^2) instead of 1.0/sin(theta)
            const iSinTheta = 1.0 / Math.sqrt(1.0 - (cosTheta * cosTheta));
            const tTheta = t * theta;

            const s0 = Math.sin(theta - tTheta);
            const s1 = Math.sin(tTheta);

            t0 = s0 * iSinTheta;
            t1 = s1 * iSinTheta;
        }
        else {
            // If q0 is nearly the same as q1 we just linearly interpolate
            t0 = 1.0 - t;
            t1 = t;
        }

        if (bFlipSign)
            t1 = -t1;

        this.x = t0 * qFrom.x;
        this.y = t0 * qFrom.y;
        this.z = t0 * qFrom.z;
        this.w = t0 * qFrom.w;

        this.x += t1 * qTo.x;
        this.y += t1 * qTo.y;
        this.z += t1 * qTo.z;
        this.w += t1 * qTo.w;

        this.Normalize();
    }

    GetRotationAxisAndAngle() {
        const acos = Math.acos(this.w);
        const d = Math.sin(acos);

        let axis: Vec3 = new Vec3();
        let angleInRadian: number;

        if (d < 0.00001) {
            axis.Set(1, 0, 0);
        }
        else {
            const invD = 1.0 / d;

            axis.x = this.x * invD;
            axis.y = this.y * invD;
            axis.z = this.z * invD;
        }

        angleInRadian = acos * 2.0;

        return { axis, angleInRadian };
    }

    IsEqualRotation(qOther: Quat, epsilon: number): boolean {
        const res1 = this.GetRotationAxisAndAngle();
        const res2 = qOther.GetRotationAxisAndAngle();

        if (Angle.IsEqualSimple(res1.angleInRadian, res2.angleInRadian, epsilon) && res1.axis.IsEqual(res2.axis, epsilon)) {
            return true;
        }

        if (Angle.IsEqualSimple(res1.angleInRadian, -res2.angleInRadian, epsilon) && res1.axis.IsEqual(res2.axis.GetNegated(), epsilon)) {
            return true;
        }

        return false;
    }

    GetAsEulerAngles() {
        let yaw: number;
        let pitch: number;
        let roll: number;

        // roll (x-axis rotation)
        const sinr = 2.0 * (this.w * this.x + this.y * this.z);
        const cosr = 1.0 - 2.0 * (this.x * this.x + this.y * this.y);
        roll = Math.atan2(sinr, cosr);

        // pitch (y-axis rotation)
        const sinp = 2.0 * (this.w * this.y - this.z * this.x);
        if (Math.abs(sinp) >= 1.0)
            pitch = Math.abs(Math.PI * 0.5) * Math.sign(sinp);
        else
            pitch = Math.asin(sinp);

        // yaw (z-axis rotation)
        const siny = 2.0 * (this.w * this.z + this.x * this.y);
        const cosy = 1.0 - 2.0 * (this.y * this.y + this.z * this.z);
        yaw = Math.atan2(siny, cosy);

        return { yaw, pitch, roll };
    }

    SetFromEulerAngles(radianX: number, radianY: number, radianZ: number): void {
        const yaw = radianZ;
        const pitch = radianY;
        const roll = radianX;
        const cy = Math.cos(yaw * 0.5);
        const sy = Math.sin(yaw * 0.5);
        const cr = Math.cos(roll * 0.5);
        const sr = Math.sin(roll * 0.5);
        const cp = Math.cos(pitch * 0.5);
        const sp = Math.sin(pitch * 0.5);

        this.w = (cy * cr * cp + sy * sr * sp);
        this.x = (cy * sr * cp - sy * cr * sp);
        this.y = (cy * cr * sp + sy * sr * cp);
        this.z = (sy * cr * cp - cy * sr * sp);
    }

    RotateVec3(vector: Vec3): void {
        // t = cross(this, vector) * 2
        const tx = (this.y * vector.z - this.z * vector.y) * 2.0;
        const ty = (this.z * vector.x - this.x * vector.z) * 2.0;
        const tz = (this.x * vector.y - this.y * vector.x) * 2.0;

        // t2 = cross(this, t)
        const t2x = this.y * tz - this.z * ty;
        const t2y = this.z * tx - this.x * tz;
        const t2z = this.x * ty - this.y * tx;

        vector.x += (tx * this.w) + t2x;
        vector.y += (ty * this.w) + t2y;
        vector.z += (tz * this.w) + t2z;
    }

    InvRotateVec3(vector: Vec3): void {
        // t = cross(this, vector) * 2
        const tx = (-this.y * vector.z + this.z * vector.y) * 2.0;
        const ty = (-this.z * vector.x + this.x * vector.z) * 2.0;
        const tz = (-this.x * vector.y + this.y * vector.x) * 2.0;

        // t2 = cross(this, t)
        const t2x = -this.y * tz + this.z * ty;
        const t2y = -this.z * tx + this.x * tz;
        const t2z = -this.x * ty + this.y * tx;

        vector.x += (tx * -this.w) + t2x;
        vector.y += (ty * -this.w) + t2y;
        vector.z += (tz * -this.w) + t2z;
    }


    /**
    * Concatenates the rotations of 'this' and 'rhs' and writes the result into 'this': 
    * this = this * rhs
    */
    ConcatenateRotations(rhs: Quat): void {
        let q: Quat = new Quat;

        q.w = this.w * rhs.w - (this.x * rhs.x + this.y * rhs.y + this.z * rhs.z);

        const t1x = rhs.x * this.w;
        const t1y = rhs.y * this.w;
        const t1z = rhs.z * this.w;

        const t2x = this.x * rhs.w;
        const t2y = this.y * rhs.w;
        const t2z = this.z * rhs.w;

        q.x = t1x + t2x;
        q.y = t1y + t2y;
        q.z = t1z + t2z;

        // q.v += Cross(this.v, q2.v)
        q.x += this.y * rhs.z - this.z * rhs.y;
        q.y += this.z * rhs.x - this.x * rhs.z;
        q.z += this.x * rhs.y - this.y * rhs.x;

        this.x = q.x;
        this.y = q.y;
        this.z = q.z;
        this.w = q.w;
    }

    SetConcatenatedRotations(lhs: Quat, rhs: Quat): void {
        this.SetQuat(lhs);
        this.ConcatenateRotations(rhs);
    }

    IsIdentical(rhs: Quat): boolean {
        return this.x == rhs.x &&
            this.y == rhs.y &&
            this.z == rhs.z &&
            this.w == rhs.w;
    }
}