module ez.foundation.math.vec3;

public import ez.foundation.types.types;
public import ez.foundation.math.random;
public import ez.foundation.math.angle;
public import ez.foundation.math.vec2;

// CODEGEN: SynthesizeTemplate("ezVec3Template<TYPE>", "ezVec3Template<float>", "ezVec3Template<double>")

extern (C++,struct) struct ezVec3Template(TYPE)
{
  // CODEGEN-BEGIN: Struct("ezVec3Template<TYPE>")
  // Operator: %
  TYPE x;
  TYPE y;
  TYPE z;
  static ezVec3Template!(TYPE) ZeroVector();
  static ezVec3Template!(TYPE) OneVector();
  static const(ezVec3Template!(TYPE)) UnitXAxis();
  static const(ezVec3Template!(TYPE)) UnitYAxis();
  static const(ezVec3Template!(TYPE)) UnitZAxis();
  const(ezVec2Template!(TYPE)) GetAsVec2() const;
  const(ezVec4Template!(TYPE)) GetAsVec4(TYPE w) const;
  const(ezVec4Template!(TYPE)) GetAsPositionVec4() const;
  const(ezVec4Template!(TYPE)) GetAsDirectionVec4() const;
  const(TYPE)* GetData() const;
  TYPE* GetData();
  void Set(TYPE xyz);
  void Set(TYPE X, TYPE Y, TYPE Z);
  void SetZero();
  TYPE GetLength() const;
  ezResult SetLength(TYPE fNewLength, TYPE fEpsilon /* = ezMath::DefaultEpsilon<Type>() */);
  TYPE GetLengthSquared() const;
  TYPE GetLengthAndNormalize();
  const(ezVec3Template!(TYPE)) GetNormalized() const;
  void Normalize();
  ezResult NormalizeIfNotZero(ref const(ezVec3Template!(TYPE)) vFallback /* = ezVec3Template<float>(1, 0, 0) */, TYPE fEpsilon /* = ezMath::SmallEpsilon() */);
  bool IsZero() const;
  bool IsZero(TYPE fEpsilon) const;
  bool IsNormalized(TYPE fEpsilon /* = ezMath::HugeEpsilon<Type>() */) const;
  bool IsNaN() const;
  bool IsValid() const;
  // Operator: -
  // Operator: +=
  // Operator: -=
  // Operator: *=
  // Operator: /=
  // Operator: *=
  // Operator: /=
  bool IsIdentical(ref const(ezVec3Template!(TYPE)) rhs) const;
  bool IsEqual(ref const(ezVec3Template!(TYPE)) rhs, TYPE fEpsilon) const;
  ezAngle GetAngleBetween(ref const(ezVec3Template!(TYPE)) rhs) const;
  TYPE Dot(ref const(ezVec3Template!(TYPE)) rhs) const;
  const(ezVec3Template!(TYPE)) CrossRH(ref const(ezVec3Template!(TYPE)) rhs) const;
  const(ezVec3Template!(TYPE)) CompMin(ref const(ezVec3Template!(TYPE)) rhs) const;
  const(ezVec3Template!(TYPE)) CompMax(ref const(ezVec3Template!(TYPE)) rhs) const;
  const(ezVec3Template!(TYPE)) CompClamp(ref const(ezVec3Template!(TYPE)) low, ref const(ezVec3Template!(TYPE)) high) const;
  const(ezVec3Template!(TYPE)) CompMul(ref const(ezVec3Template!(TYPE)) rhs) const;
  const(ezVec3Template!(TYPE)) CompDiv(ref const(ezVec3Template!(TYPE)) rhs) const;
  const(ezVec3Template!(TYPE)) Abs() const;
  ezResult CalculateNormal(ref const(ezVec3Template!(TYPE)) v1, ref const(ezVec3Template!(TYPE)) v2, ref const(ezVec3Template!(TYPE)) v3);
  void MakeOrthogonalTo(ref const(ezVec3Template!(TYPE)) vNormal);
  const(ezVec3Template!(TYPE)) GetOrthogonalVector() const;
  const(ezVec3Template!(TYPE)) GetReflectedVector(ref const(ezVec3Template!(TYPE)) vNormal) const;
  const(ezVec3Template!(TYPE)) GetRefractedVector(ref const(ezVec3Template!(TYPE)) vNormal, TYPE fRefIndex1, TYPE fRefIndex2) const;
  static ezVec3Template!(TYPE) CreateRandomPointInSphere(ref ezRandom rng);
  static ezVec3Template!(TYPE) CreateRandomDirection(ref ezRandom rng);
  static ezVec3Template!(TYPE) CreateRandomDeviationX(ref ezRandom rng, ref const(ezAngle) maxDeviation);
  static ezVec3Template!(TYPE) CreateRandomDeviationY(ref ezRandom rng, ref const(ezAngle) maxDeviation);
  static ezVec3Template!(TYPE) CreateRandomDeviationZ(ref ezRandom rng, ref const(ezAngle) maxDeviation);
  static ezVec3Template!(TYPE) CreateRandomDeviation(ref ezRandom rng, ref const(ezAngle) maxDeviation, ref const(ezVec3Template!(TYPE)) vNormal);
  // Operator: =
  // CODEGEN-END

  ////////////////////
  // D-specific code

public:
  // DMD bug workaround: small structs are passed in registers to C++, unless they have a (trivial) destructor
   ~this()
  {
  }
}

alias ezVec3 = ezVec3Template!(float);
