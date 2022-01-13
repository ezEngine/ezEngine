module ez.foundation.math.angle;

extern (C++,class) struct ezAngle
{
  // CODEGEN-BEGIN: Struct("ezAngle")
  static float DegToRadMultiplier();
  static float RadToDegMultiplier();
  static float DegToRad(float f);
  static float RadToDeg(float f);
  static ezAngle Degree(float fDegree);
  static ezAngle Radian(float fRadian);
  // Operator: %
  float GetDegree() const;
  float GetRadian() const;
  void SetRadian(float rad);
  void NormalizeRange();
  ezAngle GetNormalizedRange() const;
  static ezAngle AngleBetween(ezAngle a, ezAngle b);
  bool IsEqualSimple(ezAngle rhs, ezAngle epsilon) const;
  bool IsEqualNormalized(ezAngle rhs, ezAngle epsilon) const;
  // Operator: -
  // Operator: +
  // Operator: -
  // Operator: +=
  // Operator: -=
  // Operator: ==
  // Operator: !=
  // Operator: <
  // Operator: >
  // Operator: <=
  // Operator: >=
private:
  float m_fRadian;
  // Operator: =
  // CODEGEN-END

  ////////////////////
  // D-specific code

private:

  this(float fRadian)
  {
    m_fRadian = fRadian;
  }

public:
  // DMD bug workaround: small structs are passed in registers to C++, unless they have a (trivial) destructor
   ~this()
  {
  }

  ezAngle opUnary(string op)() if (op == "-")
  {
    return ezAngle(-m_fRadian);
  }

  ezAngle opBinary(string op)(ezAngle rhs) if (op == "-" || op == "+")
  {
    return ezAngle(mixin("m_fRadian " ~ op ~ " rhs.m_fRadian"));
  }

  void opOpAssign(string op)(ezAngle rhs) if (op == "-" || op == "+")
  {
    mixin("m_fRadian " ~ op ~ "= rhs.m_fRadian;");
  }

  bool opEquals()(auto ref const ezAngle rhs) const
  {
    return m_fRadian == rhs.m_fRadian;
  }

  int opCmp()(auto ref const ezAngle rhs) const
  {
    if (m_fRadian == rhs.m_fRadian)
      return 0;

    return m_fRadian < rhs.m_fRadian ? -1 : 1;
  }
}
