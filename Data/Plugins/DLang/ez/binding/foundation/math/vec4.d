module ez.foundation.math.vec4;

public import ez.foundation.math.vec3;

// CODEGEN: SynthesizeTemplate("ezVec4Template<TYPE>", "ezVec4Template<float>", "ezVec4Template<double>")

extern (C++,struct) struct ezVec4Template(TYPE)
{
  // CODEGEN-BEGIN: Struct("ezVec4Template<TYPE>")
  // Operator: %
  TYPE x;
  TYPE y;
  TYPE z;
  TYPE w;
  static const(ezVec4Template!(TYPE)) ZeroVector();
  static const(ezVec4Template!(TYPE)) OneVector();
  static const(ezVec4Template!(TYPE)) OriginPoint();
  static const(ezVec4Template!(TYPE)) UnitXAxis();
  static const(ezVec4Template!(TYPE)) UnitYAxis();
  static const(ezVec4Template!(TYPE)) UnitZAxis();
  static const(ezVec4Template!(TYPE)) UnitWAxis();
  const(ezVec2Template!(TYPE)) GetAsVec2() const;
  const(ezVec3Template!(TYPE)) GetAsVec3() const;
  const(TYPE)* GetData() const;
  TYPE* GetData();
  void Set(TYPE xyzw);
  void Set(TYPE X, TYPE Y, TYPE Z, TYPE W);
  void SetZero();
  TYPE GetLength() const;
  TYPE GetLengthSquared() const;
  TYPE GetLengthAndNormalize();
  const(ezVec4Template!(TYPE)) GetNormalized() const;
  void Normalize();
  ezResult NormalizeIfNotZero(ref const(ezVec4Template!(TYPE)) vFallback /* = ezVec4Template<Type>(1, 0, 0, 0) */, TYPE fEpsilon /* = ezMath::SmallEpsilon<Type>() */);
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
  bool IsIdentical(ref const(ezVec4Template!(TYPE)) rhs) const;
  bool IsEqual(ref const(ezVec4Template!(TYPE)) rhs, TYPE fEpsilon) const;
  TYPE Dot(ref const(ezVec4Template!(TYPE)) rhs) const;
  const(ezVec4Template!(TYPE)) CompMin(ref const(ezVec4Template!(TYPE)) rhs) const;
  const(ezVec4Template!(TYPE)) CompMax(ref const(ezVec4Template!(TYPE)) rhs) const;
  const(ezVec4Template!(TYPE)) CompClamp(ref const(ezVec4Template!(TYPE)) low, ref const(ezVec4Template!(TYPE)) high) const;
  const(ezVec4Template!(TYPE)) CompMul(ref const(ezVec4Template!(TYPE)) rhs) const;
  const(ezVec4Template!(TYPE)) CompDiv(ref const(ezVec4Template!(TYPE)) rhs) const;
  const(ezVec4Template!(TYPE)) Abs() const;
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

alias ezVec4 = ezVec4Template!(float);
