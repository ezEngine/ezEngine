import __Vec3 = require("./Vec3")
import __Angle = require("./Angle")

export import Vec3 = __Vec3.Vec3;
export import ezMath = __Vec3.ezMath;
export import Angle = __Angle.Angle;

export class Quat
{
    x : number;
    y : number;
    z : number;
    w : number;

    // TODO: void SetFromMat3(const ezMat3Template<Type>& m)
    // const ezMat3Template<Type> GetAsMat3() const
    // const ezMat4Template<Type> GetAsMat4() const

    constructor(_x: number = 0.0, _y: number = 0.0, _z: number = 0.0, _w: number = 1.0)
    {
        this.x = _x;
        this.y = _y;
        this.z = _z;
        this.w = _w;
    }

    Clone(): Quat
    {
        return new Quat(this.x, this.y, this.z, this.w);
    }

    SetIdentity(): void
    {
        this.x = 0;
        this.y = 0;
        this.z = 0;
        this.w = 1.0;
    }

    static IdentityQuaternion(): Quat
    {
        return new Quat();
    }
    
    Normalize(): void
    {
        let n = this.x * this.x + this.y * this.y + this.z * this.z + this.w * this.w;

        n = 1.0 / Math.sqrt(n);
      
        this.x *= n;
        this.y *= n;
        this.z *= n;
        this.w *= n;
    }

    Negate(): void
    {
        this.x = -this.x;
        this.y = -this.y;
        this.z = -this.z;
    }

    GetNegated(): Quat
    {
        return new Quat(-this.x, -this.y, -this.z, this.w);
    }

    SetFromAxisAndAngle(vRotationAxis: Vec3, angleInRadian: number)
    {
      const halfAngle = angleInRadian * 0.5;

      let sinHalfAngle = Math.sin(halfAngle);
    
      this.x = vRotationAxis.x * sinHalfAngle;
      this.y = vRotationAxis.y * sinHalfAngle;
      this.z = vRotationAxis.z * sinHalfAngle;
      this.w = Math.cos(halfAngle);
    }    

    SetShortestRotation(vDirFrom: Vec3, vDirTo: Vec3): void
    {
      const v0 = vDirFrom.GetNormalized();
      const v1 = vDirTo.GetNormalized();
    
      const fDot = v0.Dot(v1);
    
      // if both vectors are identical -> no rotation needed
      if (ezMath.IsNumberEqual(fDot, 1.0, 0.0001))
      {
        this.SetIdentity();
        return;
      }
      else if (ezMath.IsNumberEqual(fDot, -1.0, 0.0001)) // if both vectors are opposing
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

    SetSlerp(qFrom: Quat, qTo: Quat, t: number): void
    {
      const qdelta = 0.009;
    
      let cosTheta = (qFrom.x * qTo.x + qFrom.y * qTo.y + qFrom.z * qTo.z + qFrom.w * qTo.w);
    
      let bFlipSign = false;
      
      if (cosTheta < 0.0)
      {
        bFlipSign = true;
        cosTheta = -cosTheta;
      }
    
      let t0: number, t1: number;
    
      if (cosTheta < qdelta)
      {
        let theta = Math.acos(cosTheta);
    
        // use sqrtInv(1+c^2) instead of 1.0/sin(theta)
        const iSinTheta = 1.0 / Math.sqrt(1.0 - (cosTheta * cosTheta));
        const tTheta = t * theta;
    
        const s0 = Math.sin(theta - tTheta);
        const s1 = Math.sin(tTheta);
    
        t0 = s0 * iSinTheta;
        t1 = s1 * iSinTheta;
      }
      else
      {
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

    GetRotationAxisAndAngle()
    {
        const acos = Math.acos(this.w);
        const d = Math.sin(acos);

        let axis: Vec3 = new Vec3();
        let angleInRadian: number;

        if (d < 0.00001)
        {
            axis.Set(1, 0, 0);
        }
        else
        {
            const invD = 1.0 / d;
           
            axis.x = this.x * invD;
            axis.y = this.y * invD;
            axis.z = this.z * invD;
        }

        angleInRadian = acos * 2.0;

        return { axis, angleInRadian };
    }

    IsEqualRotation(qOther: Quat, epsilon: number) : boolean
    {
        const res1 = this.GetRotationAxisAndAngle();
        const res2 = qOther.GetRotationAxisAndAngle();
    
        if (Angle.IsEqualSimple(res1.angleInRadian, res2.angleInRadian, epsilon) && res1.axis.IsEqual(res2.axis, epsilon))
        {
            return true;
        }

        if (Angle.IsEqualSimple(res1.angleInRadian, -res2.angleInRadian, epsilon) && res1.axis.IsEqual(res2.axis.GetNegated(), epsilon))
        {
            return true;
        }
    
        return false;
    }    

    GetAsEulerAngles() 
    {
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

    SetFromEulerAngles(radianX: number, radianY: number, radianZ: number): void 
    {
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

    RotateVec3(vector: Vec3): void
    {
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
    
    ConcatenateRotations(q2: Quat): void
    {
        let q: Quat = new Quat;
        
        q.w = this.w * q2.w - (this.x * q2.x + this.y * q2.y + this.z * q2.z);
        
        let t1x = q2.x * this.w;
        let t1y = q2.y * this.w;
        let t1z = q2.z * this.w;
        
        let t2x = this.x * q2.w;
        let t2y = this.y * q2.w;
        let t2z = this.z * q2.w;
        
        q.x = t1x + t2x;
        q.y = t1y + t2y;
        q.z = t1z + t2z;
        
        // TODO
        // CrossRH = return new Vec3(this.y * rhs.z - this.z * rhs.y, this.z * rhs.x - this.x * rhs.z, this.x * rhs.y - this.y * rhs.x);
        q.v.AddVec3(this.v.CrossRH(q2.v));

        this.x = q.x;
        this.y = q.y;
        this.z = q.z;
        this.w = q.w;
    }
    
    IsIdentical(rhs: Quat): boolean
    {
        return this.x == rhs.x && 
               this.y == rhs.y &&
               this.z == rhs.z &&
               this.w   == rhs.w;
    }
}