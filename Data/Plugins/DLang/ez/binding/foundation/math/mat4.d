module ez.foundation.math.mat4;

public import ez.foundation.math.mat3;
public import ez.foundation.math.vec4;

// CODEGEN: SynthesizeTemplate("ezMat4Template<TYPE>", "ezMat4Template<float>", "ezMat4Template<double>")

extern(C++, struct) struct ezMat4Template(TYPE)
{
  // CODEGEN-BEGIN: Struct("ezMat4Template<TYPE>")
  // Operator: %
  TYPE[16] m_fElementsCM;
  ref TYPE Element(int column, int row);
  TYPE Element(int column, int row) const;
  // method 'SetFromArray' - unsupported argument 'layout'
  // method 'GetAsArray' - unsupported argument 'layout'
  void SetElements(TYPE c1r1, TYPE c2r1, TYPE c3r1, TYPE c4r1, TYPE c1r2, TYPE c2r2, TYPE c3r2, TYPE c4r2, TYPE c1r3, TYPE c2r3, TYPE c3r3, TYPE c4r3, TYPE c1r4, TYPE c2r4, TYPE c3r4, TYPE c4r4);
  void SetTransformationMatrix(ref const(ezMat3Template!(TYPE)) Rotation, ref const(ezVec3Template!(TYPE)) vTranslation);
  void SetZero();
  void SetIdentity();
  void SetTranslationMatrix(ref const(ezVec3Template!(TYPE)) vTranslation);
  void SetScalingMatrix(ref const(ezVec3Template!(TYPE)) s);
  void SetRotationMatrixX(ezAngle angle);
  void SetRotationMatrixY(ezAngle angle);
  void SetRotationMatrixZ(ezAngle angle);
  void SetRotationMatrix(ref const(ezVec3Template!(TYPE)) vAxis, ezAngle angle);
  static const(ezMat4Template!(TYPE)) IdentityMatrix();
  static const(ezMat4Template!(TYPE)) ZeroMatrix();
  void Transpose();
  const(ezMat4Template!(TYPE)) GetTranspose() const;
  ezResult Invert(TYPE fEpsilon /* = ezMath::SmallEpsilon<Type>() */);
  const(ezMat4Template!(TYPE)) GetInverse(TYPE fEpsilon /* = ezMath::SmallEpsilon<Type>() */) const;
  bool IsZero(TYPE fEpsilon /* = ezMath::DefaultEpsilon<Type>() */) const;
  bool IsIdentity(TYPE fEpsilon /* = ezMath::DefaultEpsilon<Type>() */) const;
  bool IsValid() const;
  bool IsNaN() const;
  ezVec4Template!(TYPE) GetRow(uint uiRow) const;
  void SetRow(uint uiRow, ref const(ezVec4Template!(TYPE)) row);
  ezVec4Template!(TYPE) GetColumn(uint uiColumn) const;
  void SetColumn(uint uiColumn, ref const(ezVec4Template!(TYPE)) column);
  ezVec4Template!(TYPE) GetDiagonal() const;
  void SetDiagonal(ref const(ezVec4Template!(TYPE)) diag);
  const(ezVec3Template!(TYPE)) GetTranslationVector() const;
  void SetTranslationVector(ref const(ezVec3Template!(TYPE)) v);
  void SetRotationalPart(ref const(ezMat3Template!(TYPE)) Rotation);
  const(ezMat3Template!(TYPE)) GetRotationalPart() const;
  const(ezVec3Template!(TYPE)) GetScalingFactors() const;
  ezResult SetScalingFactors(ref const(ezVec3Template!(TYPE)) vXYZ, TYPE fEpsilon /* = ezMath::DefaultEpsilon<Type>() */);
  const(ezVec3Template!(TYPE)) TransformPosition(ref const(ezVec3Template!(TYPE)) v) const;
  void TransformPosition(ezVec3Template!(TYPE)* inout_v, uint uiNumVectors, uint uiStride /* = sizeof(ezVec3Template<Type>) */) const;
  const(ezVec3Template!(TYPE)) TransformDirection(ref const(ezVec3Template!(TYPE)) v) const;
  void TransformDirection(ezVec3Template!(TYPE)* inout_v, uint uiNumVectors, uint uiStride /* = sizeof(ezVec3Template<Type>) */) const;
  const(ezVec4Template!(TYPE)) Transform(ref const(ezVec4Template!(TYPE)) v) const;
  void Transform(ezVec4Template!(TYPE)* inout_v, uint uiNumVectors, uint uiStride /* = sizeof(ezVec4Template<Type>) */) const;
  // Operator: *=
  // Operator: /=
  bool IsIdentical(ref const(ezMat4Template!(TYPE)) rhs) const;
  bool IsEqual(ref const(ezMat4Template!(TYPE)) rhs, TYPE fEpsilon) const;
  // Operator: =
  // CODEGEN-END
}

alias ezMat4 = ezMat4Template!(float);

