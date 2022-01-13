module ez.foundation.math.mat3;

public import ez.foundation.math.vec3;

// CODEGEN: SynthesizeTemplate("ezMat3Template<TYPE>", "ezMat3Template<float>", "ezMat3Template<double>")

extern(C++, struct) struct ezMat3Template(TYPE)
{
  // CODEGEN-BEGIN: Struct("ezMat3Template<TYPE>")
  // Operator: %
  TYPE[9] m_fElementsCM;
  ref TYPE Element(int column, int row);
  TYPE Element(int column, int row) const;
  // method 'SetFromArray' - unsupported argument 'layout'
  // method 'GetAsArray' - unsupported argument 'layout'
  void SetElements(TYPE c1r1, TYPE c2r1, TYPE c3r1, TYPE c1r2, TYPE c2r2, TYPE c3r2, TYPE c1r3, TYPE c2r3, TYPE c3r3);
  void SetZero();
  void SetIdentity();
  void SetScalingMatrix(ref const(ezVec3Template!(TYPE)) s);
  void SetRotationMatrixX(ezAngle angle);
  void SetRotationMatrixY(ezAngle angle);
  void SetRotationMatrixZ(ezAngle angle);
  void SetRotationMatrix(ref const(ezVec3Template!(TYPE)) vAxis, ezAngle angle);
  static const(ezMat3Template!(TYPE)) IdentityMatrix();
  static const(ezMat3Template!(TYPE)) ZeroMatrix();
  void Transpose();
  const(ezMat3Template!(TYPE)) GetTranspose() const;
  ezResult Invert(TYPE fEpsilon /* = ezMath::SmallEpsilon<Type>() */);
  const(ezMat3Template!(TYPE)) GetInverse(TYPE fEpsilon /* = ezMath::SmallEpsilon<Type>() */) const;
  bool IsZero(TYPE fEpsilon /* = ezMath::DefaultEpsilon<Type>() */) const;
  bool IsIdentity(TYPE fEpsilon /* = ezMath::DefaultEpsilon<Type>() */) const;
  bool IsValid() const;
  bool IsNaN() const;
  ezVec3Template!(TYPE) GetRow(uint uiRow) const;
  void SetRow(uint uiRow, ref const(ezVec3Template!(TYPE)) row);
  ezVec3Template!(TYPE) GetColumn(uint uiColumn) const;
  void SetColumn(uint uiColumn, ref const(ezVec3Template!(TYPE)) column);
  ezVec3Template!(TYPE) GetDiagonal() const;
  void SetDiagonal(ref const(ezVec3Template!(TYPE)) diag);
  const(ezVec3Template!(TYPE)) GetScalingFactors() const;
  ezResult SetScalingFactors(ref const(ezVec3Template!(TYPE)) vXYZ, TYPE fEpsilon /* = ezMath::DefaultEpsilon() */);
  TYPE GetDeterminant() const;
  const(ezVec3Template!(TYPE)) TransformDirection(ref const(ezVec3Template!(TYPE)) v) const;
  // Operator: *=
  // Operator: /=
  bool IsIdentical(ref const(ezMat3Template!(TYPE)) rhs) const;
  bool IsEqual(ref const(ezMat3Template!(TYPE)) rhs, TYPE fEpsilon) const;
  // Operator: =
  // CODEGEN-END
}

alias ezMat3 = ezMat3Template!(float);

