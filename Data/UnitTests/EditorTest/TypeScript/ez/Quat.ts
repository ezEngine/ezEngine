import __Vec3 = require("./Vec3")
import __Angle = require("./Angle")

export import Vec3 = __Vec3.Vec3;
export import ezMath = __Vec3.ezMath;
export import Angle = __Angle.Angle;

export class Quat
{
    v : Vec3;
    w : number;

    // TODO: void SetFromMat3(const ezMat3Template<Type>& m)
    // const ezMat3Template<Type> GetAsMat3() const
    // const ezMat4Template<Type> GetAsMat4() const

    constructor(_x: number = 0.0, _y: number = 0.0, _z: number = 0.0, _w: number = 1.0)
    {
        this.v = new Vec3(_x, _y, _z);
        this.w = _w;
    }

    Clone(): Quat
    {
        return new Quat(this.v.x, this.v.y, this.v.z, this.w);
    }

    SetIdentity(): void
    {
        this.v.x = 0;
        this.v.y = 0;
        this.v.z = 0;
        this.w = 1.0;
    }

    static IdentityQuaternion(): Quat
    {
        return new Quat();
    }
    
    Normalize(): void
    {
        let n = this.v.x * this.v.x + this.v.y * this.v.y + this.v.z * this.v.z + this.w * this.w;

        n = 1.0 / Math.sqrt(n);
      
        this.v.MulNumber(n);
        this.w *= n;
    }

    Negate(): void
    {
        this.v.MulNumber(-1.0);
    }

    GetNegated(): Quat
    {
        return new Quat(-this.v.x, -this.v.y, -this.v.z, this.w);
    }

    SetFromAxisAndAngle(vRotationAxis: Vec3, angleInRadian: number)
    {
      const halfAngle = angleInRadian * 0.5;
    
      this.v.SetMul(vRotationAxis, Math.sin(halfAngle));
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

      this.v.SetDiv(c, s);
      this.w = s * 0.5;
    
      this.Normalize();
    }    

    SetSlerp(qFrom: Quat, qTo: Quat, t: number): void
    {
      const qdelta = 0.009;
    
      let cosTheta = (qFrom.v.x * qTo.v.x + qFrom.v.y * qTo.v.y + qFrom.v.z * qTo.v.z + qFrom.w * qTo.w);
    
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
    
      this.v.x = t0 * qFrom.v.x;
      this.v.y = t0 * qFrom.v.y;
      this.v.z = t0 * qFrom.v.z;
      this.w = t0 * qFrom.w;
    
      this.v.x += t1 * qTo.v.x;
      this.v.y += t1 * qTo.v.y;
      this.v.z += t1 * qTo.v.z;
      this.w += t1 * qTo.w;
    
      this.Normalize();
    }    

    GetRotationAxisAndAngle()
    {
        const acos = Math.acos(this.w);
        const d = Math.sin(acos);

        let axis: Vec3;
        let angleInRadian: number;

        if (d < 0.00001)
        {
            axis.Set(1, 0, 0);
        }
        else
        {
            axis.SetDiv(this.v, d);
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
        const sinr = 2.0 * (this.w * this.v.x + this.v.y * this.v.z);
        const cosr = 1.0 - 2.0 * (this.v.x * this.v.x + this.v.y * this.v.y);
        roll = Math.atan2(sinr, cosr);

        // pitch (y-axis rotation)
        const sinp = 2.0 * (this.w * this.v.y - this.v.z * this.v.x);
        if (Math.abs(sinp) >= 1.0)
            pitch = Math.abs(Math.PI * 0.5) * Math.sign(sinp);
        else
            pitch = Math.asin(sinp);

        // yaw (z-axis rotation)
        const siny = 2.0 * (this.w * this.v.z + this.v.x * this.v.y);
        const cosy = 1.0 - 2.0 * (this.v.y * this.v.y + this.v.z * this.v.z);
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
        this.v.x = (cy * sr * cp - sy * cr * sp);
        this.v.y = (cy * cr * sp + sy * sr * cp);
        this.v.z = (sy * cr * cp - cy * sr * sp);
    }
      
    RotateVec3(vector: Vec3): void
    {
        let t = this.v.CrossRH(vector)
        t.MulNumber(2.0);

        const t2 = this.v.CrossRH(t);

        t.MulNumber(this.w);
        t.AddVec3(t2);

        vector.AddVec3(t);
    }

    ConcatenateRotations(q2: Quat): void
    {
        let q: Quat;

        q.w = this.w * q2.w - this.v.Dot(q2.v);

        let temp1: Vec3;
        temp1 = q2.v.Clone();
        temp1.MulNumber(this.w);

        let temp2: Vec3;
        temp2 = this.v.Clone();
        temp2.MulNumber(q2.w);

        q.v.SetAdd(temp1, temp2);
        q.v.AddVec3(this.v.CrossRH(q2.v));

        this.v = q.v;
        this.w = q.w;
    }
    
    IsIdentical(rhs: Quat): boolean
    {
        return this.v.x == rhs.v.x && 
               this.v.y == rhs.v.y &&
               this.v.z == rhs.v.z &&
               this.w   == rhs.w;
    }
}