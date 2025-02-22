#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>

//////////////////////////////////////////////////////////////////////////
// ezMath
//////////////////////////////////////////////////////////////////////////

void ezAngelScriptEngineSingleton::Register_Math()
{
  // static functions
  m_pEngine->SetDefaultNamespace("ezMath");

  AS_CHECK(m_pEngine->RegisterGlobalFunction("bool IsNaN(float value)", asFUNCTION(ezMath::IsNaN<float>), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("bool IsNaN(double value)", asFUNCTION(ezMath::IsNaN<double>), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("bool IsFinite(float value)", asFUNCTION(ezMath::IsFinite<float>), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("bool IsFinite(double value)", asFUNCTION(ezMath::IsFinite<double>), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Sin(ezAngle a)", asFUNCTION(ezMath::Sin), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Cos(ezAngle a)", asFUNCTION(ezMath::Cos), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Tan(ezAngle a)", asFUNCTION(ezMath::Tan), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezAngle ASin(float f)", asFUNCTION(ezMath::ASin), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezAngle ACos(float f)", asFUNCTION(ezMath::ACos), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezAngle ATan(float f)", asFUNCTION(ezMath::ATan), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezAngle ATan2(float x, float y)", asFUNCTION(ezMath::ATan2), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Exp(float f)", asFUNCTIONPR(ezMath::Exp, (float), float), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Ln(float f)", asFUNCTIONPR(ezMath::Ln, (float), float), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Log2(float f)", asFUNCTIONPR(ezMath::Log2, (float), float), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("uint32 Log2i(uint32 uiVal)", asFUNCTIONPR(ezMath::Log2i, (ezUInt32), ezUInt32), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Log10(float f)", asFUNCTIONPR(ezMath::Log10, (float), float), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Log(float fBase, float f)", asFUNCTIONPR(ezMath::Log, (float, float), float), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Pow2(float f)", asFUNCTIONPR(ezMath::Pow2, (float), float), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Pow(float fBase, float fExp)", asFUNCTIONPR(ezMath::Pow, (float, float), float), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezInt32 Pow2(ezInt32 i)", asFUNCTIONPR(ezMath::Pow2, (ezInt32), ezInt32), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezInt32 Pow(ezInt32 iBase, ezInt32 iExp)", asFUNCTIONPR(ezMath::Pow, (ezInt32, ezInt32), ezInt32), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Sqrt(float f)", asFUNCTIONPR(ezMath::Sqrt, (float), float), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("double Sqrt(double f)", asFUNCTIONPR(ezMath::Sqrt, (double), double), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Sign(float f)", asFUNCTION(ezMath::Sign<float>), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezInt32 Sign(ezInt32 f)", asFUNCTION(ezMath::Sign<ezInt32>), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Abs(float f)", asFUNCTION(ezMath::Abs<float>), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezInt32 Abs(ezInt32 f)", asFUNCTION(ezMath::Abs<ezInt32>), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezInt32 Min(ezInt32 f1, ezInt32 f2)", asFUNCTIONPR(ezMath::Min, (ezInt32, ezInt32), ezInt32), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Min(float f1, float f2)", asFUNCTIONPR(ezMath::Min, (float, float), float), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezInt32 Max(ezInt32 f1, ezInt32 f2)", asFUNCTIONPR(ezMath::Max, (ezInt32, ezInt32), ezInt32), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Max(float f1, float f2)", asFUNCTIONPR(ezMath::Max, (float, float), float), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezInt32 Clamp(ezInt32 val, ezInt32 min, ezInt32 max)", asFUNCTIONPR(ezMath::Clamp, (ezInt32, ezInt32, ezInt32), ezInt32), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Clamp(float val, float min, float max)", asFUNCTIONPR(ezMath::Clamp, (float, float, float), float), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Floor(float f)", asFUNCTIONPR(ezMath::Floor, (float), float), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Ceil(float f)", asFUNCTIONPR(ezMath::Ceil, (float), float), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezInt32 FloorToInt(float f)", asFUNCTIONPR(ezMath::FloorToInt, (float), ezInt32), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezInt32 CeilToInt(float f)", asFUNCTIONPR(ezMath::CeilToInt, (float), ezInt32), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Lerp(float from, float to, float factor)", asFUNCTIONPR(ezMath::Lerp, (float, float, float), float), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezVec2 Lerp(ezVec2 from, ezVec2 to, float factor)", asFUNCTIONPR(ezMath::Lerp, (ezVec2, ezVec2, float), ezVec2), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezVec3 Lerp(ezVec3 from, ezVec3 to, float factor)", asFUNCTIONPR(ezMath::Lerp, (ezVec3, ezVec3, float), ezVec3), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezVec4 Lerp(ezVec4 from, ezVec4 to, float factor)", asFUNCTIONPR(ezMath::Lerp, (ezVec4, ezVec4, float), ezVec4), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("ezColor Lerp(ezColor from, ezColor to, float factor)", asFUNCTIONPR(ezMath::Lerp, (ezColor, ezColor, float), ezColor), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("float Unlerp(float from, float to, float value)", asFUNCTIONPR(ezMath::Unlerp, (float, float, float), float), asCALL_CDECL));

  AS_CHECK(m_pEngine->RegisterGlobalFunction("bool IsEqual(float lhs, float rhs, float fEpsilon)", asFUNCTIONPR(ezMath::IsEqual, (float, float, float), bool), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("bool IsZero(float value, float fEpsilon)", asFUNCTIONPR(ezMath::IsZero, (float, float), bool), asCALL_CDECL));
  AS_CHECK(m_pEngine->RegisterGlobalFunction("bool IsInRange(float value, float min, float max)", asFUNCTIONPR(ezMath::IsInRange, (float, float, float), bool), asCALL_CDECL));

  // TODO AngelScript: finish ezMath registration

  /* not exposed yet:

  ezUInt32 WrapUInt(ezUInt32 uiValue, ezUInt32 uiExcludedMaxValue);
  ezInt32 WrapInt(ezInt32 iValue, ezUInt32 uiExcludedMaxValue);
  ezInt32 WrapInt(ezInt32 iValue, ezInt32 iMinValue, ezInt32 iExcludedMaxValue);
  float WrapFloat01(float fValue);
  float WrapFloat(float fValue, float fMinValue, float fMaxValue);

  T Saturate(T value);

  ezInt32 FloatToInt(float value);

  float Round(float f);
  ezInt32 RoundToInt(float f);
  double Round(double f);
  float RoundToMultiple(float f, float fMultiple);
  double RoundToMultiple(double f, double fMultiple);

  Type Fraction(Type f);

  float Mod(float value, float fDiv);
  double Mod(double f, double fDiv);

  float RoundDown(float f, float fMultiple);
  double RoundDown(double f, double fMultiple);
  float RoundUp(float f, float fMultiple);
  double RoundUp(double f, double fMultiple);

  ezInt32 RoundUp(ezInt32 value, ezUInt16 uiMultiple);
  ezInt32 RoundDown(ezInt32 value, ezUInt16 uiMultiple);
  ezUInt32 RoundUp(ezUInt32 value, ezUInt16 uiMultiple);
  ezUInt32 RoundDown(ezUInt32 value, ezUInt16 uiMultiple);

  bool IsOdd(ezInt32 i);
  bool IsEven(ezInt32 i);

  T Step(T value, T edge);
  Type SmoothStep(Type value, Type edge1, Type edge2);
  Type SmootherStep(Type value, Type edge1, Type edge2);

  bool IsPowerOf(ezInt32 value, ezInt32 iBase);
  bool IsPowerOf2(ezInt32 value);
  bool IsPowerOf2(ezUInt32 value);
  bool IsPowerOf2(ezUInt64 value);

  ezUInt32 PowerOfTwo_Floor(ezUInt32 value);
  ezUInt64 PowerOfTwo_Floor(ezUInt64 value);
  ezUInt32 PowerOfTwo_Ceil(ezUInt32 value);
  ezUInt64 PowerOfTwo_Ceil(ezUInt64 value);

  ezUInt32 GreatestCommonDivisor(ezUInt32 a, ezUInt32 b);

  ezUInt32 ColorFloatToUnsignedInt(float value);
  ezUInt8 ColorFloatToByte(float value);
  ezUInt16 ColorFloatToShort(float value);
  ezInt8 ColorFloatToSignedByte(float value);
  ezInt16 ColorFloatToSignedShort(float value);

  float ColorByteToFloat(ezUInt8 value);
  float ColorShortToFloat(ezUInt16 value);
  float ColorSignedByteToFloat(ezInt8 value);
  float ColorSignedShortToFloat(ezInt16 value);
  */

  m_pEngine->SetDefaultNamespace("");
}

//////////////////////////////////////////////////////////////////////////
// ezAngle
//////////////////////////////////////////////////////////////////////////

static int ezAngle_opCmp(const ezAngle& lhs, const ezAngle& rhs)
{
  if (lhs < rhs)
    return -1;
  if (rhs < lhs)
    return +1;

  return 0;
}

void ezAngelScriptEngineSingleton::Register_Angle()
{
  // static functions
  {
    m_pEngine->SetDefaultNamespace("ezAngle");

    AS_CHECK(m_pEngine->RegisterGlobalFunction("float DegToRad(float fDegree)", asFUNCTION(ezAngle::DegToRad<float>), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("float RadToDeg(float fRadians)", asFUNCTION(ezAngle::RadToDeg<float>), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezAngle MakeZero()", asFUNCTION(ezAngle::MakeZero), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezAngle MakeFromDegree(float fDegree)", asFUNCTION(ezAngle::MakeFromDegree), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezAngle MakeFromRadian(float fRadians)", asFUNCTION(ezAngle::MakeFromRadian), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezAngle AngleBetween(ezAngle a1, ezAngle a2)", asFUNCTION(ezAngle::AngleBetween), asCALL_CDECL));

    m_pEngine->SetDefaultNamespace("");
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "float GetDegree() const", asMETHOD(ezAngle, GetDegree), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "float GetRadian() const", asMETHOD(ezAngle, GetRadian), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "void SetRadian(float fRadians)", asMETHOD(ezAngle, SetRadian), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "void NormalizeRange()", asMETHOD(ezAngle, NormalizeRange), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "ezAngle GetNormalizedRange() const", asMETHOD(ezAngle, GetNormalizedRange), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "bool IsEqualSimple(ezAngle rhs, ezAngle epsilon) const", asMETHOD(ezAngle, IsEqualSimple), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "bool IsEqualNormalized(ezAngle rhs, ezAngle epsilon) const", asMETHOD(ezAngle, IsEqualNormalized), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "ezAngle opNeg() const", asMETHODPR(ezAngle, operator-, () const, ezAngle), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "ezAngle opAdd(ezAngle) const", asMETHODPR(ezAngle, operator+, (ezAngle) const, ezAngle), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "ezAngle opSub(ezAngle) const", asMETHODPR(ezAngle, operator-, (ezAngle) const, ezAngle), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "void opAddAssign(ezAngle)", asMETHOD(ezAngle, operator+=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "void opSubAssign(ezAngle)", asMETHOD(ezAngle, operator-=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "bool opEquals(const ezAngle& in) const", asMETHODPR(ezAngle, operator==, (const ezAngle&) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "int opCmp(const ezAngle& in) const", asFUNCTION(ezAngle_opCmp), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "ezAngle opMul(float) const", asFUNCTIONPR(operator*, (const ezAngle&, float), ezAngle), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "ezAngle opMul_r(float) const", asFUNCTIONPR(operator*, (const ezAngle&, float), ezAngle), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "ezAngle opDiv(float) const", asFUNCTIONPR(operator/, (const ezAngle&, float), ezAngle), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "float opDiv(const ezAngle& in) const", asFUNCTIONPR(operator/, (const ezAngle&, const ezAngle&), float), asCALL_CDECL_OBJFIRST));
}


//////////////////////////////////////////////////////////////////////////
// ezVec2
//////////////////////////////////////////////////////////////////////////

static int ezVec2_opCmp(const ezVec2& lhs, const ezVec2& rhs)
{
  if (lhs < rhs)
    return -1;
  if (rhs < lhs)
    return +1;

  return 0;
}

static void ezVec2_Construct1(void* pMemory, float fXyz)
{
  new (pMemory) ezVec2(fXyz);
}

static void ezVec2_Construct2(void* pMemory, float x, float y)
{
  new (pMemory) ezVec2(x, y);
}

void ezAngelScriptEngineSingleton::Register_Vec2()
{
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezVec2", "float x", asOFFSET(ezVec2, x)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezVec2", "float y", asOFFSET(ezVec2, y)));

  // static functions
  {
    m_pEngine->SetDefaultNamespace("ezVec2");
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezVec2 MakeNaN()", asFUNCTION(ezVec2::MakeNaN<float>), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezVec2 MakeZero()", asFUNCTION(ezVec2::MakeZero), asCALL_CDECL));
    m_pEngine->SetDefaultNamespace("");
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "ezVec3 GetAsVec3(float z) const", asMETHOD(ezVec2, GetAsVec3), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "ezVec4 GetAsVec4(float z, float w) const", asMETHOD(ezVec2, GetAsVec4), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "void Set(float xyz)", asMETHODPR(ezVec2, Set, (float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "void Set(float x, float y)", asMETHODPR(ezVec2, Set, (float, float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "void SetZero()", asMETHOD(ezVec2, SetZero), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "float GetLength() const", asMETHODPR(ezVec2, GetLength, () const, float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "float GetDistanceTo(const ezVec2& in rhs) const", asMETHODPR(ezVec2, GetDistanceTo, (const ezVec2&) const, float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "float GetSquaredDistanceTo(const ezVec2& in rhs) const", asMETHODPR(ezVec2, GetSquaredDistanceTo, (const ezVec2&) const, float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "float GetLengthSquared() const", asMETHOD(ezVec2, GetLengthSquared), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "float GetLengthAndNormalize()", asMETHODPR(ezVec2, GetLengthAndNormalize, (), float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "ezVec2 GetNormalized() const", asMETHODPR(ezVec2, GetNormalized, () const, const ezVec2), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "void Normalize()", asMETHODPR(ezVec2, Normalize, (), void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "bool IsZero() const", asMETHODPR(ezVec2, IsZero, () const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "bool IsZero(float fEpsilon) const", asMETHODPR(ezVec2, IsZero, (float) const, bool), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "bool IsNormalized(float fEpsilon = 0.001f) const", asMETHODPR(ezVec2, IsNormalized, (float) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "bool IsNaN() const", asMETHOD(ezVec2, IsNaN), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "bool IsValid() const", asMETHOD(ezVec2, IsValid), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "void opAddAssign(const ezVec2& in)", asMETHOD(ezVec2, operator+=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "void opSubAssign(const ezVec2& in)", asMETHOD(ezVec2, operator-=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "void opMulAssign(float)", asMETHODPR(ezVec2, operator*=, (float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "void opDivAssign(float)", asMETHODPR(ezVec2, operator/=, (float), void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "bool IsIdentical(const ezVec2& in) const", asMETHOD(ezVec2, IsIdentical), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "bool IsEqual(const ezVec2& in, float fEpsilon) const", asMETHOD(ezVec2, IsEqual), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "float Dot(const ezVec2& in) const", asMETHOD(ezVec2, Dot), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "ezVec2 CompMin(const ezVec2& in rhs) const", asMETHOD(ezVec2, CompMin), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "ezVec2 CompMax(const ezVec2& in rhs) const", asMETHOD(ezVec2, CompMax), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "ezVec2 CompClamp(const ezVec2& in rhs) const", asMETHOD(ezVec2, CompClamp), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "ezVec2 CompMul(const ezVec2& in rhs) const", asMETHOD(ezVec2, CompMul), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "ezVec2 CompDiv(const ezVec2& in rhs) const", asMETHOD(ezVec2, CompDiv), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "ezVec2 Abs() const", asMETHOD(ezVec2, Abs), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "void MakeOrthogonalTo(const ezVec2& in)", asMETHODPR(ezVec2, MakeOrthogonalTo, (const ezVec2&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "ezVec2 GetOrthogonalVector() const", asMETHOD(ezVec2, GetOrthogonalVector), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "ezVec2 GetReflectedVector(const ezVec2& in) const", asMETHODPR(ezVec2, GetReflectedVector, (const ezVec2&) const, const ezVec2), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "ezVec2 opNeg() const", asMETHODPR(ezVec2, operator-, () const, const ezVec2), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "ezVec2 opAdd(const ezVec2& in) const", asFUNCTIONPR(operator+, (const ezVec2&, const ezVec2&), const ezVec2), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "ezVec2 opSub(const ezVec2& in) const", asFUNCTIONPR(operator-, (const ezVec2&, const ezVec2&), const ezVec2), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "ezVec2 opMul(float) const", asFUNCTIONPR(operator*, (const ezVec2&, float), const ezVec2), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "ezVec2 opMul_r(float) const", asFUNCTIONPR(operator*, (float, const ezVec2&), const ezVec2), asCALL_CDECL_OBJLAST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "ezVec2 opDiv(float) const", asFUNCTIONPR(operator/, (const ezVec2&, float), const ezVec2), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "bool opEquals(const ezVec2& in) const", asFUNCTIONPR(operator==, (const ezVec2&, const ezVec2&), bool), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec2", "int opCmp(const ezVec2& in) const", asFUNCTIONPR(ezVec2_opCmp, (const ezVec2&, const ezVec2&), int), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezVec2", asBEHAVE_CONSTRUCT, "void f(float x, float y)", asFUNCTION(ezVec2_Construct2), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezVec2", asBEHAVE_CONSTRUCT, "void f(float xyz)", asFUNCTION(ezVec2_Construct1), asCALL_CDECL_OBJFIRST));
}


//////////////////////////////////////////////////////////////////////////
// ezVec3
//////////////////////////////////////////////////////////////////////////

static int ezVec3_opCmp(const ezVec3& lhs, const ezVec3& rhs)
{
  if (lhs < rhs)
    return -1;
  if (rhs < lhs)
    return +1;

  return 0;
}

static void ezVec3_Construct1(void* pMemory, float fXyz)
{
  new (pMemory) ezVec3(fXyz);
}
static void ezVec3_Construct3(void* pMemory, float x, float y, float z)
{
  new (pMemory) ezVec3(x, y, z);
}

void ezAngelScriptEngineSingleton::Register_Vec3()
{
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezVec3", "float x", asOFFSET(ezVec3, x)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezVec3", "float y", asOFFSET(ezVec3, y)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezVec3", "float z", asOFFSET(ezVec3, z)));

  // static functions
  {
    m_pEngine->SetDefaultNamespace("ezVec3");

    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezVec3 MakeNaN()", asFUNCTION(ezVec3::MakeNaN<float>), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezVec3 MakeZero()", asFUNCTION(ezVec3::MakeZero), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezVec3 MakeAxisX()", asFUNCTION(ezVec3::MakeAxisX), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezVec3 MakeAxisY()", asFUNCTION(ezVec3::MakeAxisY), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezVec3 MakeAxisZ()", asFUNCTION(ezVec3::MakeAxisZ), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezVec3 Make(float x, float y, float z)", asFUNCTION(ezVec3::Make), asCALL_CDECL));

    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezVec3 MakeRandomDirection(ezRandom& inout rng)", asFUNCTION(ezVec3::MakeRandomDirection<float>), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezVec3 MakeRandomPointInSphere(ezRandom& inout rng)", asFUNCTION(ezVec3::MakeRandomPointInSphere<float>), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezVec3 MakeRandomDeviationX(ezRandom& inout rng, const ezAngle& in maxDeviation)", asFUNCTION(ezVec3::MakeRandomDeviationX<float>), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezVec3 MakeRandomDeviationY(ezRandom& inout rng, const ezAngle& in maxDeviation)", asFUNCTION(ezVec3::MakeRandomDeviationY<float>), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezVec3 MakeRandomDeviationZ(ezRandom& inout rng, const ezAngle& in maxDeviation)", asFUNCTION(ezVec3::MakeRandomDeviationZ<float>), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezVec3 MakeRandomDeviation(ezRandom& inout rng, const ezAngle& in maxDeviation, const ezVec3& in normal)", asFUNCTION(ezVec3::MakeRandomDeviation<float>), asCALL_CDECL));

    m_pEngine->SetDefaultNamespace("");
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec2 GetAsVec2() const", asMETHOD(ezVec3, GetAsVec2), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec4 GetAsVec4(float w) const", asMETHOD(ezVec3, GetAsVec4), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec4 GetAsPositionVec4() const", asMETHOD(ezVec3, GetAsPositionVec4), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec4 GetAsDirectionVec4() const", asMETHOD(ezVec3, GetAsDirectionVec4), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "void Set(float xyz)", asMETHODPR(ezVec3, Set, (float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "void Set(float x, float y, float z)", asMETHODPR(ezVec3, Set, (float, float, float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "void SetZero()", asMETHOD(ezVec3, SetZero), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "float GetLength() const", asMETHODPR(ezVec3, GetLength, () const, float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "float GetDistanceTo(const ezVec3& in rhs) const", asMETHODPR(ezVec3, GetDistanceTo, (const ezVec3&) const, float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "float GetSquaredDistanceTo(const ezVec3& in rhs) const", asMETHODPR(ezVec3, GetSquaredDistanceTo, (const ezVec3&) const, float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "float GetLengthSquared() const", asMETHOD(ezVec3, GetLengthSquared), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "float GetLengthAndNormalize()", asMETHODPR(ezVec3, GetLengthAndNormalize, (), float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec3 GetNormalized() const", asMETHODPR(ezVec3, GetNormalized, () const, const ezVec3), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "void Normalize()", asMETHODPR(ezVec3, Normalize, (), void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "bool IsZero() const", asMETHODPR(ezVec3, IsZero, () const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "bool IsZero(float fEpsilon) const", asMETHODPR(ezVec3, IsZero, (float) const, bool), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "bool IsNormalized(float fEpsilon = 0.001f) const", asMETHODPR(ezVec3, IsNormalized, (float) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "bool IsNaN() const", asMETHOD(ezVec3, IsNaN), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "bool IsValid() const", asMETHOD(ezVec3, IsValid), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "void opAddAssign(const ezVec3& in)", asMETHOD(ezVec3, operator+=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "void opSubAssign(const ezVec3& in)", asMETHOD(ezVec3, operator-=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "void opMulAssign(const ezVec3& in)", asMETHODPR(ezVec3, operator*=, (const ezVec3&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "void opDivAssign(const ezVec3& in)", asMETHODPR(ezVec3, operator/=, (const ezVec3&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "void opMulAssign(float)", asMETHODPR(ezVec3, operator*=, (float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "void opDivAssign(float)", asMETHODPR(ezVec3, operator/=, (float), void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "bool IsIdentical(const ezVec3& in) const", asMETHOD(ezVec3, IsIdentical), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "bool IsEqual(const ezVec3& in, float fEpsilon) const", asMETHOD(ezVec3, IsEqual), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "float Dot(const ezVec3& in) const", asMETHOD(ezVec3, Dot), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec3 CrossRH(const ezVec3& in) const", asMETHOD(ezVec3, CrossRH), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec3 CompMin(const ezVec3& in) const", asMETHOD(ezVec3, CompMin), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec3 CompMax(const ezVec3& in) const", asMETHOD(ezVec3, CompMax), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec3 CompClamp(const ezVec3& in) const", asMETHOD(ezVec3, CompClamp), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec3 CompMul(const ezVec3& in) const", asMETHOD(ezVec3, CompMul), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec3 CompDiv(const ezVec3& in) const", asMETHOD(ezVec3, CompDiv), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec3 Abs() const", asMETHOD(ezVec3, Abs), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "void MakeOrthogonalTo(const ezVec3& in)", asMETHODPR(ezVec3, MakeOrthogonalTo, (const ezVec3&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec3 GetOrthogonalVector() const", asMETHODPR(ezVec3, GetOrthogonalVector, () const, const ezVec3), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec3 GetReflectedVector(const ezVec3& in) const", asMETHODPR(ezVec3, GetReflectedVector, (const ezVec3&) const, const ezVec3), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec3 opNeg() const", asMETHODPR(ezVec3, operator-, () const, const ezVec3), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec3 opAdd(const ezVec3& in) const", asFUNCTIONPR(operator+, (const ezVec3&, const ezVec3&), const ezVec3), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec3 opSub(const ezVec3& in) const", asFUNCTIONPR(operator-, (const ezVec3&, const ezVec3&), const ezVec3), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec3 opMul(float) const", asFUNCTIONPR(operator*, (const ezVec3&, float), const ezVec3), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec3 opMul_r(float) const", asFUNCTIONPR(operator*, (float, const ezVec3&), const ezVec3), asCALL_CDECL_OBJLAST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "ezVec3 opDiv(float) const", asFUNCTIONPR(operator/, (const ezVec3&, float), const ezVec3), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "bool opEquals(const ezVec3& in) const", asFUNCTIONPR(operator==, (const ezVec3&, const ezVec3&), bool), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec3", "int opCmp(const ezVec3& in) const", asFUNCTIONPR(ezVec3_opCmp, (const ezVec3&, const ezVec3&), int), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezVec3", asBEHAVE_CONSTRUCT, "void f(float x, float y, float z)", asFUNCTION(ezVec3_Construct3), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezVec3", asBEHAVE_CONSTRUCT, "void f(float xyz)", asFUNCTION(ezVec3_Construct1), asCALL_CDECL_OBJFIRST));
}


//////////////////////////////////////////////////////////////////////////
// ezVec4
//////////////////////////////////////////////////////////////////////////

static int ezVec4_opCmp(const ezVec4& lhs, const ezVec4& rhs)
{
  if (lhs < rhs)
    return -1;
  if (rhs < lhs)
    return +1;

  return 0;
}

static void ezVec4_Construct1(void* pMemory, float fXyzw)
{
  new (pMemory) ezVec4(fXyzw);
}
static void ezVec4_Construct4(void* pMemory, float x, float y, float z, float w)
{
  new (pMemory) ezVec4(x, y, z, w);
}

void ezAngelScriptEngineSingleton::Register_Vec4()
{
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezVec4", "float x", asOFFSET(ezVec4, x)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezVec4", "float y", asOFFSET(ezVec4, y)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezVec4", "float z", asOFFSET(ezVec4, z)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezVec4", "float w", asOFFSET(ezVec4, w)));

  // static functions
  {
    m_pEngine->SetDefaultNamespace("ezVec4");
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezVec4 MakeNaN()", asFUNCTION(ezVec4::MakeNaN<float>), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezVec4 MakeZero()", asFUNCTION(ezVec4::MakeZero), asCALL_CDECL));
    m_pEngine->SetDefaultNamespace("");
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "ezVec2 GetAsVec2() const", asMETHOD(ezVec4, GetAsVec2), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "ezVec3 GetAsVec3() const", asMETHOD(ezVec4, GetAsVec3), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "void Set(float xyzw)", asMETHODPR(ezVec4, Set, (float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "void Set(float x, float y, float z, float w)", asMETHODPR(ezVec4, Set, (float, float, float, float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "void SetZero()", asMETHOD(ezVec4, SetZero), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "float GetLength() const", asMETHODPR(ezVec4, GetLength, () const, float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "float GetLengthSquared() const", asMETHOD(ezVec4, GetLengthSquared), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "float GetLengthAndNormalize()", asMETHODPR(ezVec4, GetLengthAndNormalize, (), float), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "ezVec4 GetNormalized() const", asMETHODPR(ezVec4, GetNormalized, () const, const ezVec4), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "void Normalize()", asMETHODPR(ezVec4, Normalize, (), void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "bool IsZero() const", asMETHODPR(ezVec4, IsZero, () const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "bool IsZero(float fEpsilon) const", asMETHODPR(ezVec4, IsZero, (float) const, bool), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "bool IsNormalized(float fEpsilon = 0.001f) const", asMETHODPR(ezVec4, IsNormalized, (float) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "bool IsNaN() const", asMETHOD(ezVec4, IsNaN), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "bool IsValid() const", asMETHOD(ezVec4, IsValid), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "void opAddAssign(const ezVec4& in)", asMETHOD(ezVec4, operator+=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "void opSubAssign(const ezVec4& in)", asMETHOD(ezVec4, operator-=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "void opMulAssign(float)", asMETHODPR(ezVec4, operator*=, (float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "void opDivAssign(float)", asMETHODPR(ezVec4, operator/=, (float), void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "bool IsIdentical(const ezVec4& in) const", asMETHOD(ezVec4, IsIdentical), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "bool IsEqual(const ezVec4& in, float) const", asMETHOD(ezVec4, IsEqual), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "float Dot(const ezVec4& in) const", asMETHOD(ezVec4, Dot), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "ezVec4 CompMin(const ezVec4& in) const", asMETHOD(ezVec4, CompMin), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "ezVec4 CompMax(const ezVec4& in) const", asMETHOD(ezVec4, CompMax), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "ezVec4 CompClamp(const ezVec4& in) const", asMETHOD(ezVec4, CompClamp), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "ezVec4 CompMul(const ezVec4& in) const", asMETHOD(ezVec4, CompMul), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "ezVec4 CompDiv(const ezVec4& in) const", asMETHOD(ezVec4, CompDiv), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "ezVec4 Abs() const", asMETHOD(ezVec4, Abs), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "ezVec4 opNeg() const", asMETHODPR(ezVec4, operator-, () const, const ezVec4), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "ezVec4 opAdd(const ezVec4& in) const", asFUNCTIONPR(operator+, (const ezVec4&, const ezVec4&), const ezVec4), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "ezVec4 opSub(const ezVec4& in) const", asFUNCTIONPR(operator-, (const ezVec4&, const ezVec4&), const ezVec4), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "ezVec4 opMul(float) const", asFUNCTIONPR(operator*, (const ezVec4&, float), const ezVec4), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "ezVec4 opMul_r(float) const", asFUNCTIONPR(operator*, (float, const ezVec4&), const ezVec4), asCALL_CDECL_OBJLAST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "ezVec4 opDiv(float) const", asFUNCTIONPR(operator/, (const ezVec4&, float), const ezVec4), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "bool opEquals(const ezVec4& in) const", asFUNCTIONPR(operator==, (const ezVec4&, const ezVec4&), bool), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezVec4", "int opCmp(const ezVec4& in) const", asFUNCTIONPR(ezVec4_opCmp, (const ezVec4&, const ezVec4&), int), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezVec4", asBEHAVE_CONSTRUCT, "void f(float x, float y, float z, float w)", asFUNCTION(ezVec4_Construct4), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezVec4", asBEHAVE_CONSTRUCT, "void f(float xyzw)", asFUNCTION(ezVec4_Construct1), asCALL_CDECL_OBJFIRST));
}

//////////////////////////////////////////////////////////////////////////
// ezQuat
//////////////////////////////////////////////////////////////////////////

void ezAngelScriptEngineSingleton::Register_Quat()
{
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezQuat", "float x", asOFFSET(ezQuat, x)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezQuat", "float y", asOFFSET(ezQuat, y)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezQuat", "float z", asOFFSET(ezQuat, z)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezQuat", "float w", asOFFSET(ezQuat, w)));

  // static functions
  {
    m_pEngine->SetDefaultNamespace("ezQuat");

    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezQuat MakeIdentity()", asFUNCTION(ezQuat::MakeIdentity), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezQuat MakeFromElements(float x, float y, float z, float w)", asFUNCTION(ezQuat::MakeFromElements), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezQuat MakeFromAxisAndAngle(const ezVec3& in vAxis , ezAngle angle)", asFUNCTION(ezQuat::MakeFromAxisAndAngle), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezQuat MakeShortestRotation(const ezVec3& in vFrom, const ezVec3& in vTo)", asFUNCTION(ezQuat::MakeShortestRotation), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezQuat MakeFromMat3(const ezMat3& in)", asFUNCTION(ezQuat::MakeFromMat3), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezQuat MakeSlerp(const ezQuat& in qFrom, const ezQuat& in qTo, float fFactor)", asFUNCTION(ezQuat::MakeSlerp), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezQuat MakeFromEulerAngles(ezAngle x, ezAngle y, ezAngle z)", asFUNCTION(ezQuat::MakeFromEulerAngles), asCALL_CDECL));

    m_pEngine->SetDefaultNamespace("");
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "void SetIdentity()", asMETHOD(ezQuat, SetIdentity), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "void ReconstructFromMat3(const ezMat3& in)", asMETHOD(ezQuat, ReconstructFromMat3), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "void ReconstructFromMat4(const ezMat3& in)", asMETHOD(ezQuat, ReconstructFromMat4), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "void Normalize()", asMETHOD(ezQuat, Normalize), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "void GetRotationAxisAndAngle(ezVec3& out, ezAngle& out, float fEpsilon = 0.00001) const", asMETHOD(ezQuat, GetRotationAxisAndAngle), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "ezVec3 GetVectorPart() const", asMETHOD(ezQuat, GetVectorPart), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "ezMat3 GetAsMat3() const", asMETHOD(ezQuat, GetAsMat3), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "ezMat3 GetAsMat4() const", asMETHOD(ezQuat, GetAsMat4), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "bool IsValid(float fEpsilon = 0.00001) const", asMETHOD(ezQuat, IsValid), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "bool IsNaN() const", asMETHOD(ezQuat, IsNaN), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "bool IsEqualRotation(const ezQuat& in, float fEpsilon = 0.00001) const", asMETHOD(ezQuat, IsEqualRotation), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "void Invert()", asMETHOD(ezQuat, Invert), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "ezQuat GetInverse() const", asMETHOD(ezQuat, GetInverse), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "ezQuat GetNegated() const", asMETHOD(ezQuat, GetNegated), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "float Dot(const ezQuat& in) const", asMETHOD(ezQuat, Dot), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "ezVec3 Rotate(const ezVec3& in) const", asMETHOD(ezQuat, Rotate), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "void GetAsEulerAngles(float& out, float& out, float& out) const", asMETHOD(ezQuat, GetAsEulerAngles), asCALL_THISCALL));


  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "ezQuat opMul(const ezQuat& in) const", asFUNCTIONPR(operator*, (const ezQuat&, const ezQuat&), const ezQuat), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "ezVec3 opMul(const ezVec3& in) const", asFUNCTIONPR(operator*, (const ezQuat&, const ezVec3&), const ezVec3), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezQuat", "bool opEquals(const ezQuat& in) const", asFUNCTIONPR(operator==, (const ezQuat&, const ezQuat&), bool), asCALL_CDECL_OBJFIRST));
}


//////////////////////////////////////////////////////////////////////////
// ezTransform
//////////////////////////////////////////////////////////////////////////

static void ezTransform_Construct3(void* pMemory, const ezVec3& v, const ezQuat& r, const ezVec3& s)
{
  new (pMemory) ezTransform(v, r, s);
}

void ezAngelScriptEngineSingleton::Register_Transform()
{
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezTransform", "ezVec3 m_vPosition", asOFFSET(ezTransform, m_vPosition)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezTransform", "ezQuat m_qRotation", asOFFSET(ezTransform, m_qRotation)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezTransform", "ezVec3 m_vScale", asOFFSET(ezTransform, m_vScale)));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezTransform", asBEHAVE_CONSTRUCT, "void f(const ezVec3& in vPosition, const ezQuat& in qRotation = ezQuat::MakeIdentity(), const ezVec3& in vScale = ezVec3(1))", asFUNCTION(ezTransform_Construct3), asCALL_CDECL_OBJFIRST));

  // static functions
  {
    m_pEngine->SetDefaultNamespace("ezTransform");

    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTransform Make(const ezVec3& in vPosition, const ezQuat& in qRotation = ezQuat::MakeIdentity(), const ezVec3& in vScale = ezVec3(1))", asFUNCTION(ezTransform::Make), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTransform MakeIdentity()", asFUNCTION(ezTransform::MakeIdentity), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTransform MakeFromMat4(const ezMat4& in)", asFUNCTION(ezTransform::MakeFromMat4), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTransform MakeLocalTransform(const ezTransform& in, const ezTransform& in)", asFUNCTION(ezTransform::MakeLocalTransform), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTransform MakeGlobalTransform(const ezTransform& in, const ezTransform& in)", asFUNCTION(ezTransform::MakeGlobalTransform), asCALL_CDECL));

    m_pEngine->SetDefaultNamespace("");
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "void SetIdentity()", asMETHOD(ezTransform, SetIdentity), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "float GetMaxScale() const", asMETHOD(ezTransform, GetMaxScale), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "bool HasMirrorScaling() const", asMETHOD(ezTransform, HasMirrorScaling), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "bool ContainsUniformScale() const", asMETHOD(ezTransform, ContainsUniformScale), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "bool IsValid() const", asMETHOD(ezTransform, IsValid), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "bool IsIdentical(const ezTransform& in) const", asMETHOD(ezTransform, IsIdentical), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "bool IsEqual(const ezTransform& in, float fEpsilon) const", asMETHOD(ezTransform, IsEqual), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "void Invert()", asMETHOD(ezTransform, Invert), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "ezTransform GetInverse() const", asMETHOD(ezTransform, GetInverse), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "ezVec3 TransformPosition(const ezVec3& in vPosition) const", asMETHOD(ezTransform, TransformPosition), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "ezVec3 TransformDirection(const ezVec3& in vDirection) const", asMETHOD(ezTransform, TransformDirection), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "void opAddAssign(const ezVec3& in)", asMETHOD(ezTransform, operator+=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "void opSubAssign(const ezVec3& in)", asMETHOD(ezTransform, operator-=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "ezMat4 GetAsMat4() const", asMETHOD(ezTransform, GetAsMat4), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "ezVec3 opMul(const ezVec3& in) const", asFUNCTIONPR(operator*, (const ezTransform&, const ezVec3&), const ezVec3), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "ezTransform opMul_r(const ezQuat& in qRotation) const", asFUNCTIONPR(operator*, (const ezQuat&, const ezTransform&), const ezTransform), asCALL_CDECL_OBJLAST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "ezTransform opMul(const ezQuat& in qRotation) const", asFUNCTIONPR(operator*, (const ezTransform&, const ezQuat&), const ezTransform), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "ezTransform opAdd(const ezVec3& in) const", asFUNCTIONPR(operator+, (const ezTransform&, const ezVec3&), const ezTransform), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "ezTransform opSub(const ezVec3& in) const", asFUNCTIONPR(operator-, (const ezTransform&, const ezVec3&), const ezTransform), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "ezTransform opMul(const ezTransform& in) const", asFUNCTIONPR(operator*, (const ezTransform&, const ezTransform&), const ezTransform), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTransform", "bool opEquals(const ezTransform& in) const", asFUNCTIONPR(operator==, (const ezTransform&, const ezTransform&), bool), asCALL_CDECL_OBJFIRST));
}

//////////////////////////////////////////////////////////////////////////
// ezMat3
//////////////////////////////////////////////////////////////////////////

void ezAngelScriptEngineSingleton::Register_Mat3()
{
  // static functions
  {
    m_pEngine->SetDefaultNamespace("ezMat3");
    m_pEngine->SetDefaultNamespace("");
  }

  // TODO AngelScript: Register ezMat3
}

//////////////////////////////////////////////////////////////////////////
// ezMat4
//////////////////////////////////////////////////////////////////////////

void ezAngelScriptEngineSingleton::Register_Mat4()
{
  // static functions
  {
    m_pEngine->SetDefaultNamespace("ezMat4");
    m_pEngine->SetDefaultNamespace("");
  }

  // TODO AngelScript: Register ezMat4
}
