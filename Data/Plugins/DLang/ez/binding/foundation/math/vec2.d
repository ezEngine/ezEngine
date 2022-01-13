module ez.foundation.math.vec2;

public import ez.foundation.types.types;
public import ez.foundation.math.vec3;
public import ez.foundation.math.vec4;

// CODEGEN: SynthesizeTemplate("ezVec2Template<TYPE>", "ezVec2Template<float>", "ezVec2Template<double>")

extern(C++, struct) struct ezVec2Template(TYPE)
{
  // CODEGEN-BEGIN: Struct("ezVec2Template<TYPE>")
  // Operator: %
  TYPE x;
  TYPE y;
  static const(ezVec2Template!(TYPE)) ZeroVector();
  const(ezVec3Template!(TYPE)) GetAsVec3(TYPE z) const;
  const(ezVec4Template!(TYPE)) GetAsVec4(TYPE z, TYPE w) const;
  const(TYPE)* GetData() const;
  TYPE* GetData();
  void Set(TYPE xy);
  void Set(TYPE X, TYPE Y);
  void SetZero();
  TYPE GetLength() const;
  TYPE GetLengthSquared() const;
  TYPE GetLengthAndNormalize();
  const(ezVec2Template!(TYPE)) GetNormalized() const;
  void Normalize();
  ezResult NormalizeIfNotZero(ref const(ezVec2Template!(TYPE)) vFallback /* = ezVec2Template<Type>(1, 0) */, TYPE fEpsilon /* = ezMath::DefaultEpsilon<Type>() */);
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
  bool IsIdentical(ref const(ezVec2Template!(TYPE)) rhs) const;
  bool IsEqual(ref const(ezVec2Template!(TYPE)) rhs, TYPE fEpsilon) const;
  ezAngle GetAngleBetween(ref const(ezVec2Template!(TYPE)) rhs) const;
  TYPE Dot(ref const(ezVec2Template!(TYPE)) rhs) const;
  const(ezVec2Template!(TYPE)) CompMin(ref const(ezVec2Template!(TYPE)) rhs) const;
  const(ezVec2Template!(TYPE)) CompMax(ref const(ezVec2Template!(TYPE)) rhs) const;
  const(ezVec2Template!(TYPE)) CompClamp(ref const(ezVec2Template!(TYPE)) low, ref const(ezVec2Template!(TYPE)) high) const;
  const(ezVec2Template!(TYPE)) CompMul(ref const(ezVec2Template!(TYPE)) rhs) const;
  const(ezVec2Template!(TYPE)) CompDiv(ref const(ezVec2Template!(TYPE)) rhs) const;
  const(ezVec2Template!(TYPE)) Abs() const;
  void MakeOrthogonalTo(ref const(ezVec2Template!(TYPE)) vNormal);
  const(ezVec2Template!(TYPE)) GetOrthogonalVector() const;
  const(ezVec2Template!(TYPE)) GetReflectedVector(ref const(ezVec2Template!(TYPE)) vNormal) const;
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

alias ezVec2 = ezVec2Template!(float);

