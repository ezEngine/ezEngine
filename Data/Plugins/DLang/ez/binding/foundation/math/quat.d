module ez.foundation.math.quat;

public import ez.foundation.math.vec3;
public import ez.foundation.math.angle;
public import ez.foundation.math.mat3;
public import ez.foundation.math.mat4;

// CODEGEN: SynthesizeTemplate("ezQuatTemplate<TYPE>", "ezQuatTemplate<float>", "ezQuatTemplate<double>")

extern(C++, class) struct ezQuatTemplate(TYPE)
{
  // CODEGEN-BEGIN: Struct("ezQuatTemplate<TYPE>")
  // Operator: %
  ezVec3Template!(TYPE) v;
  TYPE w;
  static const(ezQuatTemplate!(TYPE)) IdentityQuaternion();
  void SetIdentity();
  void SetElements(TYPE X, TYPE Y, TYPE Z, TYPE W);
  void SetFromAxisAndAngle(ref const(ezVec3Template!(TYPE)) vRotationAxis, ezAngle angle);
  void SetShortestRotation(ref const(ezVec3Template!(TYPE)) vDirFrom, ref const(ezVec3Template!(TYPE)) vDirTo);
  void SetFromMat3(ref const(ezMat3Template!(TYPE)) m);
  void SetSlerp(ref const(ezQuatTemplate!(TYPE)) qFrom, ref const(ezQuatTemplate!(TYPE)) qTo, TYPE t);
  void Normalize();
  ezResult GetRotationAxisAndAngle(ref ezVec3Template!(TYPE) vAxis, ref ezAngle angle, TYPE fEpsilon /* = ezMath::DefaultEpsilon() */) const;
  const(ezMat3Template!(TYPE)) GetAsMat3() const;
  const(ezMat4Template!(TYPE)) GetAsMat4() const;
  bool IsValid(TYPE fEpsilon /* = ezMath::DefaultEpsilon<Type>() */) const;
  bool IsNaN() const;
  bool IsEqualRotation(ref const(ezQuatTemplate!(TYPE)) qOther, TYPE fEpsilon) const;
  // Operator: -
  TYPE Dot(ref const(ezQuatTemplate!(TYPE)) rhs) const;
  void GetAsEulerAngles(ref ezAngle out_x, ref ezAngle out_y, ref ezAngle out_z) const;
  void SetFromEulerAngles(ref const(ezAngle) x, ref const(ezAngle) y, ref const(ezAngle) z);
  // Operator: =
  // CODEGEN-END
}

alias ezQuat = ezQuatTemplate!(float);