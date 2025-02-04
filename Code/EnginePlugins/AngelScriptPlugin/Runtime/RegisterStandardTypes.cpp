#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AngelScriptEngineSingleton.h>
#include <Core/World/Declarations.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Foundation/Strings/HashedString.h>

void ezAngelScriptEngineSingleton::RegisterStandardTypes()
{
  EZ_LOG_BLOCK("AS::RegisterStandardTypes");

  AS_CHECK(m_pEngine->RegisterTypedef("ezInt8", "int8"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezInt16", "int16"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezInt32", "int32"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezInt64", "int64"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezUInt8", "uint8"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezUInt16", "uint16"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezUInt32", "uint32"));
  AS_CHECK(m_pEngine->RegisterTypedef("ezUInt64", "uint64"));

  AS_CHECK(m_pEngine->RegisterObjectType("ezRTTI", 0, asOBJ_REF | asOBJ_NOCOUNT));

  // TODO AngelScript: ezResult ?

  RegisterPodValueType<ezVec2>();
  RegisterPodValueType<ezVec3>();
  RegisterPodValueType<ezVec4>();
  RegisterPodValueType<ezAngle>();
  RegisterPodValueType<ezQuat>();
  RegisterPodValueType<ezMat3>();
  RegisterPodValueType<ezMat4>();
  RegisterPodValueType<ezTransform>();
  RegisterPodValueType<ezTime>();
  RegisterPodValueType<ezColor>();
  RegisterPodValueType<ezColorGammaUB>();
  RegisterPodValueType<ezStringView>();
  RegisterPodValueType<ezGameObjectHandle>();
  RegisterPodValueType<ezComponentHandle>();
  RegisterPodValueType<ezTempHashedString>();
  RegisterPodValueType<ezHashedString>();

  RegisterNonPodValueType<ezString>();
  RegisterNonPodValueType<ezStringBuilder>();

  RegisterRefType<ezGameObject>();
  RegisterRefType<ezComponent>();
  RegisterRefType<ezWorld>();
  RegisterRefType<ezMessage>();
  RegisterRefType<ezClock>();

  {
    AS_CHECK(m_pEngine->RegisterObjectType("ezRandom", 0, asOBJ_REF | asOBJ_NOCOUNT));
    AddForbiddenType("ezRandom");
  }

  Register_RTTI();
  Register_Vec2();
  Register_Vec3();
  Register_Vec4();
  Register_Angle();
  Register_Quat();
  Register_Transform();
  Register_GameObject();
  Register_Time();
  Register_Mat3();
  Register_Mat4();
  Register_World();
  Register_Clock();
  Register_StringView();
  Register_String();
  Register_StringBuilder();
  Register_HashedString();
  Register_TempHashedString();
  Register_Color();
  Register_ColorGammaUB();
  Register_Random();
  Register_Math();

  // TODO AngelScript: register these standard types
  // ezBoundingBox
  // ezBoundingSphere
  // ezPlane
}

//////////////////////////////////////////////////////////////////////////
// ezRTTI
//////////////////////////////////////////////////////////////////////////

const ezRTTI* ezRTTI_GetType(ezStringView name)
{
  return ezRTTI::FindTypeByName(name);
}

void ezAngelScriptEngineSingleton::Register_RTTI()
{
  // static functions
  {
    m_pEngine->SetDefaultNamespace("ezRTTI");

    AS_CHECK(m_pEngine->RegisterGlobalFunction("const ezRTTI@ GetType(ezStringView)", asFUNCTION(ezRTTI_GetType), asCALL_CDECL));

    m_pEngine->SetDefaultNamespace("");
  }
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

static void ezVec2_Construct1(void* memory, float xyz)
{
  new (memory) ezVec2(xyz);
}

static void ezVec2_Construct2(void* memory, float x, float y)
{
  new (memory) ezVec2(x, y);
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

static void ezVec3_Construct1(void* memory, float xyz)
{
  new (memory) ezVec3(xyz);
}
static void ezVec3_Construct3(void* memory, float x, float y, float z)
{
  new (memory) ezVec3(x, y, z);
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

static void ezVec4_Construct1(void* memory, float xyzw)
{
  new (memory) ezVec4(xyzw);
}
static void ezVec4_Construct4(void* memory, float x, float y, float z, float w)
{
  new (memory) ezVec4(x, y, z, w);
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
// ezGameObject
//////////////////////////////////////////////////////////////////////////

void ezGameObject_TryGetComponentOfBaseType(asIScriptGeneric* gen)
{
  ezGameObject* pObj = (ezGameObject*)gen->GetObject();
  int typeId = gen->GetArgTypeId(0);

  if (auto info = gen->GetEngine()->GetTypeInfoById(typeId))
  {
    if (const ezRTTI* pRtti = static_cast<const ezRTTI*>(info->GetUserData(ezAsUserData::RttiPtr)))
    {
      ezComponent* pComponent;
      if (pObj->TryGetComponentOfBaseType(pRtti, pComponent))
      {
        ezComponent** ref = (ezComponent**)gen->GetArgAddress(0);
        *ref = pComponent;

        gen->SetReturnByte(1);
        return;
      }
    }
  }

  gen->SetReturnByte(0);
}

void ezGameObject_CreateComponent(asIScriptGeneric* gen)
{
  ezGameObject* pObj = (ezGameObject*)gen->GetObject();
  ezWorld* pWorld = pObj->GetWorld();
  const int typeId = gen->GetArgTypeId(0);

  if (auto info = gen->GetEngine()->GetTypeInfoById(typeId))
  {
    if (const ezRTTI* pRtti = static_cast<const ezRTTI*>(info->GetUserData(ezAsUserData::RttiPtr)))
    {
      if (auto pCompMan = pWorld->GetOrCreateManagerForComponentType(pRtti))
      {
        ezComponentHandle hComp = pCompMan->CreateComponent(pObj);

        ezComponent* pComponent;
        if (pWorld->TryGetComponent(hComp, pComponent))
        {
          ezComponent** ref = (ezComponent**)gen->GetArgAddress(0);
          *ref = pComponent;

          gen->SetReturnByte(1);
          return;
        }
      }
    }
  }

  gen->SetReturnByte(0);
}

void ezGameObjectHandle_Construct(void* memory)
{
  new (memory) ezGameObjectHandle();
}

void ezComponentHandle_Construct(void* memory)
{
  new (memory) ezComponentHandle();
}

void ezAngelScriptEngineSingleton::Register_GameObject()
{
  Register_EnumType(ezGetStaticRTTI<ezObjectMsgQueueType>());
  Register_EnumType(ezGetStaticRTTI<ezTransformPreservation>());

  {
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezGameObjectHandle", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ezGameObjectHandle_Construct), asCALL_CDECL_OBJFIRST));

    AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObjectHandle", "void Invalidate()", asMETHOD(ezGameObjectHandle, Invalidate), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObjectHandle", "bool IsInvalidated() const", asMETHOD(ezGameObjectHandle, IsInvalidated), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObjectHandle", "bool opEquals(ezGameObjectHandle) const", asMETHODPR(ezGameObjectHandle, operator==, (ezGameObjectHandle) const, bool), asCALL_THISCALL));
  }

  {
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezComponentHandle", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ezComponentHandle_Construct), asCALL_CDECL_OBJFIRST));

    AS_CHECK(m_pEngine->RegisterObjectMethod("ezComponentHandle", "void Invalidate()", asMETHOD(ezComponentHandle, Invalidate), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezComponentHandle", "bool IsInvalidated() const", asMETHOD(ezComponentHandle, IsInvalidated), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezComponentHandle", "bool opEquals(ezComponentHandle) const", asMETHODPR(ezComponentHandle, operator==, (ezComponentHandle) const, bool), asCALL_THISCALL));
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezGameObjectHandle GetHandle() const", asMETHOD(ezGameObject, GetHandle), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void MakeDynamic()", asMETHOD(ezGameObject, MakeDynamic), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void MakeStatic()", asMETHOD(ezGameObject, MakeStatic), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool IsDynamic() const", asMETHOD(ezGameObject, IsDynamic), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool IsStatic() const", asMETHOD(ezGameObject, IsStatic), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetActiveFlag(bool bActive)", asMETHOD(ezGameObject, SetActiveFlag), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool GetActiveFlag() const", asMETHOD(ezGameObject, GetActiveFlag), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool IsActive() const", asMETHOD(ezGameObject, IsActive), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetName(ezStringView sName)", asMETHODPR(ezGameObject, SetName, (ezStringView), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetName(const ezHashedString& in sName)", asMETHODPR(ezGameObject, SetName, (const ezHashedString&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezStringView GetName() const", asMETHOD(ezGameObject, GetName), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool HasName(const ezTempHashedString& in sName) const", asMETHOD(ezGameObject, HasName), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetGlobalKey(ezStringView sKey)", asMETHODPR(ezGameObject, SetGlobalKey, (ezStringView), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetGlobalKey(const ezHashedString& in sKey)", asMETHODPR(ezGameObject, SetGlobalKey, (const ezHashedString&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezStringView GetGlobalKey() const", asMETHOD(ezGameObject, GetGlobalKey), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetParent(const ezGameObjectHandle& in hParent, ezTransformPreservation preserve = ezTransformPreservation::PreserveGlobal)", asMETHOD(ezGameObject, SetParent), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezGameObject@ GetParent() const", asMETHODPR(ezGameObject, GetParent, () const, const ezGameObject*), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezGameObject@ GetParent()", asMETHODPR(ezGameObject, GetParent, (), ezGameObject*), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void AddChild(const ezGameObjectHandle& in hChild, ezTransformPreservation preserve = ezTransformPreservation::PreserveGlobal)", asMETHOD(ezGameObject, AddChild), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void DetachChild(const ezGameObjectHandle& in hChild, ezTransformPreservation preserve = ezTransformPreservation::PreserveGlobal)", asMETHOD(ezGameObject, DetachChild), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "uint32 GetChildCount()", asMETHOD(ezGameObject, GetChildCount), asCALL_THISCALL));
  // AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void GetChildren()", asMETHOD(ezGameObject, GetChildren), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezGameObject@ FindChildByName(const ezTempHashedString& in sName, bool bRecursive = true)", asMETHOD(ezGameObject, FindChildByName), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezGameObject@ FindChildByPath(ezStringView sPath)", asMETHOD(ezGameObject, FindChildByPath), asCALL_THISCALL));
  // AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezGameObject@ SearchForChildByNameSequence(string)", asMETHOD(ezGameObject, SearchForChildByNameSequence), asCALL_THISCALL));
  // AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezGameObject@ SearchForChildrenByNameSequence(string)", asMETHOD(ezGameObject, SearchForChildrenByNameSequence), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezWorld@ GetWorld()", asMETHODPR(ezGameObject, GetWorld, (), ezWorld*), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "const ezWorld@ GetWorld() const", asMETHODPR(ezGameObject, GetWorld, () const, const ezWorld*), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetLocalPosition(const ezVec3& in)", asMETHODPR(ezGameObject, SetLocalPosition, (ezVec3), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezVec3 GetLocalPosition() const", asMETHOD(ezGameObject, GetLocalPosition), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetLocalRotation(const ezQuat& in)", asMETHODPR(ezGameObject, SetLocalRotation, (ezQuat), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezQuat GetLocalRotation() const", asMETHOD(ezGameObject, GetLocalRotation), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetLocalScaling(const ezVec3& in)", asMETHODPR(ezGameObject, SetLocalScaling, (ezVec3), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezVec3 GetLocalScaling() const", asMETHOD(ezGameObject, GetLocalScaling), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetLocalUniformScaling(float fScale)", asMETHODPR(ezGameObject, SetLocalUniformScaling, (float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "float GetLocalUniformScaling() const", asMETHOD(ezGameObject, GetLocalUniformScaling), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezTransform GetLocalTransform() const", asMETHOD(ezGameObject, GetLocalTransform), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetGlobalPosition(const ezVec3& in)", asMETHODPR(ezGameObject, SetGlobalPosition, (const ezVec3&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezVec3 GetGlobalPosition() const", asMETHOD(ezGameObject, GetGlobalPosition), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetGlobalRotation(const ezQuat& in)", asMETHODPR(ezGameObject, SetGlobalRotation, (const ezQuat&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetGlobalScaling(const ezVec3& in)", asMETHODPR(ezGameObject, SetGlobalScaling, (const ezVec3&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezVec3 GetGlobalScaling() const", asMETHOD(ezGameObject, GetGlobalScaling), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetGlobalTransform(const ezTransform& in)", asMETHODPR(ezGameObject, SetGlobalTransform, (const ezTransform&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezTransform GetGlobalTransform() const", asMETHOD(ezGameObject, GetGlobalTransform), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezTransform GetLastGlobalTransform() const", asMETHOD(ezGameObject, GetLastGlobalTransform), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezVec3 GetGlobalDirForwards() const", asMETHOD(ezGameObject, GetGlobalDirForwards), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezVec3 GetGlobalDirRight() const", asMETHOD(ezGameObject, GetGlobalDirRight), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezVec3 GetGlobalDirUp() const", asMETHOD(ezGameObject, GetGlobalDirUp), asCALL_THISCALL));

#if EZ_ENABLED(EZ_GAMEOBJECT_VELOCITY)
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezVec3 GetLinearVelocity() const", asMETHOD(ezGameObject, GetLinearVelocity), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezVec3 GetAngularVelocity() const", asMETHOD(ezGameObject, GetAngularVelocity), asCALL_THISCALL));
#endif

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void UpdateGlobalTransform()", asMETHOD(ezGameObject, UpdateGlobalTransform), asCALL_THISCALL));

  // AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezBoundingBoxSphere GetLocalBounds() const", asMETHOD(ezGameObject, GetLocalBounds), asCALL_THISCALL));
  // AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezBoundingBoxSphere GetGlobalBounds() const", asMETHOD(ezGameObject, GetGlobalBounds), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void UpdateLocalBounds()", asMETHOD(ezGameObject, UpdateLocalBounds), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void UpdateGlobalBounds()", asMETHOD(ezGameObject, UpdateGlobalBounds), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void UpdateGlobalTransformAndBounds()", asMETHOD(ezGameObject, UpdateGlobalTransformAndBounds), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool TryGetComponentOfBaseType(const ezRTTI@ pType, ezComponent@& out pComponent)", asMETHODPR(ezGameObject, TryGetComponentOfBaseType, (const ezRTTI*, ezComponent*&), bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool TryGetComponentOfBaseType(?& out pTypedComponent)", asFUNCTION(ezGameObject_TryGetComponentOfBaseType), asCALL_GENERIC));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool CreateComponent(?& out pTypedComponent)", asFUNCTION(ezGameObject_CreateComponent), asCALL_GENERIC));

  // AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void GetComponents()", asMETHOD(ezGameObject, GetComponents), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "uint16 GetComponentVersion()", asMETHOD(ezGameObject, GetComponentVersion), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool SendMessage(ezMessage& inout)", asMETHODPR(ezGameObject, SendMessage, (ezMessage&), bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool SendMessage(ezMessage& inout) const", asMETHODPR(ezGameObject, SendMessage, (ezMessage&) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool SendMessageRecursive(ezMessage& inout)", asMETHODPR(ezGameObject, SendMessageRecursive, (ezMessage&), bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool SendMessageRecursive(ezMessage& inout) const", asMETHODPR(ezGameObject, SendMessageRecursive, (ezMessage&) const, bool), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void PostMessage(const ezMessage& in, ezTime delay, ezObjectMsgQueueType delivery = ezObjectMsgQueueType::NextFrame) const", asMETHOD(ezGameObject, PostMessage), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void PostMessageRecursive(const ezMessage& in, ezTime delay, ezObjectMsgQueueType delivery = ezObjectMsgQueueType::NextFrame) const", asMETHOD(ezGameObject, PostMessageRecursive), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool SendEventMessage(ezMessage& inout, const ezComponent@ pSender)", asMETHODPR(ezGameObject, SendEventMessage, (ezMessage&, const ezComponent*), bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool SendEventMessage(ezMessage& inout, const ezComponent@ pSender) const", asMETHODPR(ezGameObject, SendEventMessage, (ezMessage&, const ezComponent*) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void PostEventMessage(const ezMessage& in msg, const ezComponent@ pSender, ezTime delay, ezObjectMsgQueueType delivery = ezObjectMsgQueueType::NextFrame) const", asMETHOD(ezGameObject, PostEventMessage), asCALL_THISCALL));

  // GetTags
  // SetTags
  // SetTag
  // RemoveTag
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool HasTag(const ezTempHashedString& in sTagName) const", asMETHOD(ezGameObject, HasTag), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "uint16 GetTeamID()", asMETHOD(ezGameObject, GetTeamID), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetTeamID(uint16 id)", asMETHOD(ezGameObject, SetTeamID), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "uint32 GetStableRandomSeed()", asMETHOD(ezGameObject, GetStableRandomSeed), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetStableRandomSeed(uint32 seed)", asMETHOD(ezGameObject, SetStableRandomSeed), asCALL_THISCALL));

  // GetVisibilityState
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
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "bool opEquals(ezAngle) const", asMETHODPR(ezAngle, operator==, (const ezAngle&) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "int opCmp(ezAngle) const", asFUNCTIONPR(ezAngle_opCmp, (const ezAngle&, const ezAngle&), int), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "ezAngle opMul(float) const", asFUNCTIONPR(operator*, (ezAngle, float), ezAngle), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "ezAngle opDiv(float) const", asFUNCTIONPR(operator/, (ezAngle, float), ezAngle), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezAngle", "float opDiv(ezAngle) const", asFUNCTIONPR(operator/, (ezAngle, ezAngle), float), asCALL_CDECL_OBJFIRST));
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
// ezTime
//////////////////////////////////////////////////////////////////////////

static int ezTime_opCmp(const ezTime& lhs, const ezTime& rhs)
{
  if (lhs < rhs)
    return -1;
  if (rhs < lhs)
    return +1;

  return 0;
}

void ezAngelScriptEngineSingleton::Register_Time()
{
  // static functions
  {
    m_pEngine->SetDefaultNamespace("ezTime");

    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTime Now()", asFUNCTION(ezTime::Now), asCALL_CDECL));

    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTime MakeFromNanoseconds(double fNanoSeconds)", asFUNCTION(ezTime::MakeFromNanoseconds), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTime Nanoseconds(double fNanoSeconds)", asFUNCTION(ezTime::Nanoseconds), asCALL_CDECL));

    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTime MakeFromMicroseconds(double fMicroSeconds)", asFUNCTION(ezTime::MakeFromMicroseconds), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTime Microseconds(double fMicroSeconds)", asFUNCTION(ezTime::Microseconds), asCALL_CDECL));

    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTime MakeFromMilliseconds(double fMilliSeconds)", asFUNCTION(ezTime::MakeFromMilliseconds), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTime Milliseconds(double fMilliSeconds)", asFUNCTION(ezTime::Milliseconds), asCALL_CDECL));

    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTime MakeFromSeconds(double fSeconds)", asFUNCTION(ezTime::MakeFromSeconds), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTime Seconds(double fSeconds)", asFUNCTION(ezTime::Seconds), asCALL_CDECL));

    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTime MakeFromMinutes(double fMinutes)", asFUNCTION(ezTime::MakeFromMinutes), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTime Minutes(double fMinutes)", asFUNCTION(ezTime::Minutes), asCALL_CDECL));

    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTime MakeFromHours(double fHours)", asFUNCTION(ezTime::MakeFromHours), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTime Hours(double fHours)", asFUNCTION(ezTime::Hours), asCALL_CDECL));

    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTime MakeZero()", asFUNCTION(ezTime::MakeZero), asCALL_CDECL));

    m_pEngine->SetDefaultNamespace("");
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "bool IsZero() const", asMETHOD(ezTime, IsZero), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "bool IsNegative() const", asMETHOD(ezTime, IsNegative), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "bool IsPositive() const", asMETHOD(ezTime, IsPositive), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "bool IsZeroOrNegative() const", asMETHOD(ezTime, IsZeroOrNegative), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "bool IsZeroOrPositive() const", asMETHOD(ezTime, IsZeroOrPositive), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "float AsFloatInSeconds() const", asMETHOD(ezTime, AsFloatInSeconds), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "double GetNanoseconds() const", asMETHOD(ezTime, GetNanoseconds), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "double GetMicroseconds() const", asMETHOD(ezTime, GetMicroseconds), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "double GetMilliseconds() const", asMETHOD(ezTime, GetMilliseconds), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "double GetSeconds() const", asMETHOD(ezTime, GetSeconds), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "double GetMinutes() const", asMETHOD(ezTime, GetMinutes), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "double GetHours() const", asMETHOD(ezTime, GetHours), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "void opSubAssign(const ezTime& in)", asMETHOD(ezTime, operator-=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "void opAddAssign(const ezTime& in)", asMETHOD(ezTime, operator+=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "void opMulAssign(double)", asMETHOD(ezTime, operator*=), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "void opDivAssign(double)", asMETHOD(ezTime, operator/=), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "ezTime opSub(const ezTime& in) const", asMETHODPR(ezTime, operator-, (const ezTime&) const, ezTime), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "ezTime opAdd(const ezTime& in) const", asMETHODPR(ezTime, operator+, (const ezTime&) const, ezTime), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "ezTime opNeg() const", asMETHODPR(ezTime, operator-, () const, ezTime), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "int opCmp(const ezTime& in) const", asFUNCTIONPR(ezTime_opCmp, (const ezTime&, const ezTime&), int), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "bool opEquals(const ezTime& in) const", asMETHODPR(ezTime, operator==, (const ezTime&) const, bool), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "ezTime opMul(double) const", asFUNCTIONPR(operator*, (ezTime, double), ezTime), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "ezTime opMul_r(double) const", asFUNCTIONPR(operator*, (double, ezTime), ezTime), asCALL_CDECL_OBJLAST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "ezTime opMul(const ezTime& in) const", asFUNCTIONPR(operator*, (ezTime, ezTime), ezTime), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "ezTime opDiv(double) const", asFUNCTIONPR(operator/, (ezTime, double), ezTime), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "ezTime opDiv_r(double) const", asFUNCTIONPR(operator/, (double, ezTime), ezTime), asCALL_CDECL_OBJLAST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "ezTime opDiv(const ezTime& in) const", asFUNCTIONPR(operator/, (ezTime, ezTime), ezTime), asCALL_CDECL_OBJFIRST));
}


//////////////////////////////////////////////////////////////////////////
// ezTransform
//////////////////////////////////////////////////////////////////////////

static void ezTransform_Construct1(void* memory, const ezVec3& v)
{
  new (memory) ezTransform(v);
}

static void ezTransform_Construct2(void* memory, const ezVec3& v, const ezQuat& r)
{
  new (memory) ezTransform(v, r);
}

static void ezTransform_Construct3(void* memory, const ezVec3& v, const ezQuat& r, const ezVec3& s)
{
  new (memory) ezTransform(v, r, s);
}

void ezAngelScriptEngineSingleton::Register_Transform()
{
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezTransform", "ezVec3 m_vPosition", asOFFSET(ezTransform, m_vPosition)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezTransform", "ezQuat m_qRotation", asOFFSET(ezTransform, m_qRotation)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezTransform", "ezVec3 m_vScale", asOFFSET(ezTransform, m_vScale)));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezTransform", asBEHAVE_CONSTRUCT, "void f(const ezVec3& in vPosition)", asFUNCTION(ezTransform_Construct1), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezTransform", asBEHAVE_CONSTRUCT, "void f(const ezVec3& in vPosition, const ezQuat& in qRotation)", asFUNCTION(ezTransform_Construct2), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezTransform", asBEHAVE_CONSTRUCT, "void f(const ezVec3& in vPosition, const ezQuat& in qRotation, const ezVec3& in vScale)", asFUNCTION(ezTransform_Construct3), asCALL_CDECL_OBJFIRST));

  // static functions
  {
    m_pEngine->SetDefaultNamespace("ezTransform");

    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezTransform Make(const ezVec3& in vPosition, const ezQuat& in qRotation, const ezVec3& in vScale)", asFUNCTION(ezTransform::Make), asCALL_CDECL));
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

//////////////////////////////////////////////////////////////////////////
// ezWorld
//////////////////////////////////////////////////////////////////////////

static void ezGameObjectDesc_Construct(void* memory)
{
  new (memory) ezGameObjectDesc();
}

void ezWorld_TryGetComponent(asIScriptGeneric* gen)
{
  ezWorld* pWorld = (ezWorld*)gen->GetObject();
  ezComponentHandle* hComponent = (ezComponentHandle*)gen->GetArgObject(0);
  int typeId = gen->GetArgTypeId(1);

  if (auto info = gen->GetEngine()->GetTypeInfoById(typeId))
  {
    if (const ezRTTI* pRtti = static_cast<const ezRTTI*>(info->GetUserData(ezAsUserData::RttiPtr)))
    {
      ezComponent* pComponent;
      if (pWorld->TryGetComponent(*hComponent, pComponent))
      {
        ezComponent** ref = (ezComponent**)gen->GetArgAddress(1);
        *ref = pComponent;

        gen->SetReturnByte((*ref)->GetDynamicRTTI()->IsDerivedFrom(pRtti) ? 1 : 0);
        return;
      }
    }
  }

  gen->SetReturnByte(0);
}

void ezAngelScriptEngineSingleton::Register_World()
{
  {
    AS_CHECK(m_pEngine->RegisterObjectType("ezGameObjectDesc", sizeof(ezGameObjectDesc), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<ezGameObjectDesc>()));

    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "bool m_bActiveFlag", asOFFSET(ezGameObjectDesc, m_bActiveFlag)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "bool m_bDynamic", asOFFSET(ezGameObjectDesc, m_bDynamic)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "uint16 m_uiTeamID", asOFFSET(ezGameObjectDesc, m_uiTeamID)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "ezHashedString m_sName", asOFFSET(ezGameObjectDesc, m_sName)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "ezGameObjectHandle m_hParent", asOFFSET(ezGameObjectDesc, m_hParent)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "ezVec3 m_LocalPosition", asOFFSET(ezGameObjectDesc, m_LocalPosition)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "ezQuat m_LocalRotation", asOFFSET(ezGameObjectDesc, m_LocalRotation)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "ezVec3 m_LocalScaling", asOFFSET(ezGameObjectDesc, m_LocalScaling)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "float m_LocalUniformScaling", asOFFSET(ezGameObjectDesc, m_LocalUniformScaling)));
    // AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "ezTagSet m_Tags", asOFFSET(ezGameObjectDesc, m_Tags)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "uint32 m_uiStableRandomSeed", asOFFSET(ezGameObjectDesc, m_uiStableRandomSeed)));

    AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezGameObjectDesc", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ezGameObjectDesc_Construct), asCALL_CDECL_OBJFIRST));
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "ezStringView GetName()", asMETHOD(ezWorld, GetName), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "ezGameObjectHandle CreateObject(ezGameObjectDesc& in desc)", asMETHODPR(ezWorld, CreateObject, (const ezGameObjectDesc& desc), ezGameObjectHandle), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "ezGameObjectHandle CreateObject(ezGameObjectDesc& in desc, ezGameObject& out object)", asMETHODPR(ezWorld, CreateObject, (const ezGameObjectDesc& desc, ezGameObject*&), ezGameObjectHandle), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "void DeleteObjectDelayed(const ezGameObjectHandle& in hObject, bool bAlsoDeleteEmptyParents = true)", asMETHOD(ezWorld, DeleteObjectDelayed), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "bool IsValidObject(const ezGameObjectHandle& in hObject)", asMETHOD(ezWorld, IsValidObject), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "bool TryGetObject(ezGameObjectHandle& in, ezGameObject@& out pObject)", asMETHODPR(ezWorld, TryGetObject, (const ezGameObjectHandle&, ezGameObject*&), bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "bool TryGetObject(ezGameObjectHandle& in, const ezGameObject@& out pObject) const", asMETHODPR(ezWorld, TryGetObject, (const ezGameObjectHandle&, const ezGameObject*&) const, bool), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "bool TryGetObjectWithGlobalKey(const ezTempHashedString& in sGlobalKey, ezGameObject@& out pObject)", asMETHODPR(ezWorld, TryGetObjectWithGlobalKey, (const ezTempHashedString& sGlobalKey, ezGameObject*& out_pObject), bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "bool TryGetObjectWithGlobalKey(const ezTempHashedString& in sGlobalKey, const ezGameObject@& out pObject)", asMETHODPR(ezWorld, TryGetObjectWithGlobalKey, (const ezTempHashedString& sGlobalKey, const ezGameObject*& out_pObject) const, bool), asCALL_THISCALL));

  // GetOrCreateModule

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "bool IsValidComponent(ezComponentHandle& in)", asMETHOD(ezWorld, IsValidComponent), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "bool TryGetComponent(const ezComponentHandle& in hComponent, ?& out component)", asFUNCTION(ezWorld_TryGetComponent), asCALL_GENERIC));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "void SendMessage(const ezGameObjectHandle& in hReceiverObject, ezMessage& inout msg)", asMETHODPR(ezWorld, SendMessage, (const ezGameObjectHandle&, ezMessage&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "void SendMessageRecursive(const ezGameObjectHandle& in hReceiverObject, ezMessage& inout msg)", asMETHODPR(ezWorld, SendMessageRecursive, (const ezGameObjectHandle&, ezMessage&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "void PostMessage(const ezGameObjectHandle& in hReceiverObject, const ezMessage& in msg, ezTime delay, ezObjectMsgQueueType queueType = ezObjectMsgQueueType::NextFrame) const", asMETHODPR(ezWorld, PostMessage, (const ezGameObjectHandle&, const ezMessage&, ezTime, ezObjectMsgQueueType::Enum) const, void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "void PostMessageRecursive(const ezGameObjectHandle& in hReceiverObject, const ezMessage& in msg, ezTime delay, ezObjectMsgQueueType queueType = ezObjectMsgQueueType::NextFrame) const", asMETHODPR(ezWorld, PostMessageRecursive, (const ezGameObjectHandle&, const ezMessage&, ezTime, ezObjectMsgQueueType::Enum) const, void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "void SendMessage(const ezComponentHandle& in hReceiverObject, ezMessage& inout msg)", asMETHODPR(ezWorld, SendMessage, (const ezComponentHandle&, ezMessage&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "void PostMessage(const ezComponentHandle& in hReceiverObject, const ezMessage& in msg, ezTime delay, ezObjectMsgQueueType queueType = ezObjectMsgQueueType::NextFrame) const", asMETHODPR(ezWorld, PostMessage, (const ezComponentHandle&, const ezMessage&, ezTime, ezObjectMsgQueueType::Enum) const, void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "ezClock@ GetClock()", asMETHODPR(ezWorld, GetClock, (), ezClock&), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "const ezClock@ GetClock() const", asMETHODPR(ezWorld, GetClock, () const, const ezClock&),
    asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "ezRandom@ GetRandomNumberGenerator()", asMETHOD(ezWorld, GetRandomNumberGenerator), asCALL_THISCALL));
}

//////////////////////////////////////////////////////////////////////////
// ezClock
//////////////////////////////////////////////////////////////////////////

void ezAngelScriptEngineSingleton::Register_Clock()
{
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezClock", "void SetPaused(bool)", asMETHOD(ezClock, SetPaused), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezClock", "bool GetPaused() const", asMETHOD(ezClock, GetPaused), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezClock", "ezTime GetTimeDiff() const", asMETHOD(ezClock, GetTimeDiff), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezClock", "void SetSpeed(double)", asMETHOD(ezClock, SetSpeed), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezClock", "double GetSpeed() const", asMETHOD(ezClock, GetSpeed), asCALL_THISCALL));
}

//////////////////////////////////////////////////////////////////////////
// ezStringBase
//////////////////////////////////////////////////////////////////////////

template <typename T>
void RegisterStringBase(asIScriptEngine* pEngine, const char* szType)
{
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool StartsWith(ezStringView) const", asMETHOD(T, StartsWith), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool StartsWith_NoCase(ezStringView) const", asMETHOD(T, StartsWith_NoCase), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool EndsWith(ezStringView) const", asMETHOD(T, EndsWith), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool EndsWith_NoCase(ezStringView) const", asMETHOD(T, EndsWith_NoCase), asCALL_THISCALL));

  // FindSubString
  // FindSubString_NoCase
  // FindLastSubString
  // FindLastSubString_NoCase
  // FindWholeWord
  // FindWholeWord_NoCase

  AS_CHECK(pEngine->RegisterObjectMethod(szType, "int Compare(ezStringView) const", asMETHOD(T, Compare), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "int Compare_NoCase(ezStringView) const", asMETHOD(T, Compare_NoCase), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "int CompareN(ezStringView, uint32) const", asMETHOD(T, CompareN), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "int CompareN_NoCase(ezStringView, uint32) const", asMETHOD(T, CompareN_NoCase), asCALL_THISCALL));

  AS_CHECK(pEngine->RegisterObjectMethod(szType, "uint32 GetElementCount() const", asMETHOD(T, GetElementCount), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool IsEmpty() const", asMETHOD(T, IsEmpty), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool IsEqual(ezStringView) const", asMETHOD(T, IsEqual), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool IsEqual_NoCase(ezStringView) const", asMETHOD(T, IsEqual_NoCase), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool IsEqualN(ezStringView, uint32) const", asMETHOD(T, IsEqualN), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool IsEqualN_NoCase(ezStringView, uint32) const", asMETHOD(T, IsEqualN_NoCase), asCALL_THISCALL));

  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool HasAnyExtension() const", asMETHOD(T, HasAnyExtension), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool HasExtension(ezStringView) const", asMETHOD(T, HasExtension), asCALL_THISCALL));

  AS_CHECK(pEngine->RegisterObjectMethod(szType, "ezStringView GetFileExtension(bool full = false) const", asMETHOD(T, GetFileExtension), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "ezStringView GetFileName() const", asMETHOD(T, GetFileName), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "ezStringView GetFileNameAndExtension() const", asMETHOD(T, GetFileNameAndExtension), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "ezStringView GetFileDirectory() const", asMETHOD(T, GetFileDirectory), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool IsAbsolutePath() const", asMETHOD(T, IsAbsolutePath), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool IsRelativePath() const", asMETHOD(T, IsRelativePath), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool IsRootedPath() const", asMETHOD(T, IsRootedPath), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool GetRootedPathRootName() const", asMETHOD(T, GetRootedPathRootName), asCALL_THISCALL));
}

//////////////////////////////////////////////////////////////////////////
// ezStringView
//////////////////////////////////////////////////////////////////////////

static int ezStringView_opCmp(ezStringView lhs, ezStringView rhs)
{
  if (lhs < rhs)
    return -1;
  if (rhs < lhs)
    return +1;

  return 0;
}

static void ezStringView_opAssignStr(ezStringView* lhs, const std::string& rhs)
{
  *lhs = rhs.c_str();
}

static void StdString_opAssignStringView(std::string* lhs, ezStringView rhs)
{
  lhs->assign(rhs.GetStartPointer(), rhs.GetElementCount());
}

static void ezStringView_ConstructString(void* memory, const ezString& rhs)
{
  new (memory) ezStringView(rhs);
}

static void ezStringView_opAssignString(ezStringView* lhs, const ezString& rhs)
{
  *lhs = rhs;
}

static void ezStringView_ConstructStringBuilder(void* memory, const ezStringBuilder& rhs)
{
  new (memory) ezStringView(rhs);
}

static void ezStringView_opAssignStringBuilder(ezStringView* lhs, const ezStringBuilder& rhs)
{
  *lhs = rhs;
}

void ezAngelScriptEngineSingleton::Register_StringView()
{
  RegisterStringBase<ezStringView>(m_pEngine, "ezStringView");

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringView", "void Shrink(ezUInt32 uiShrinkCharsFront, ezUInt32 uiShrinkCharsBack)", asMETHOD(ezStringView, Shrink), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringView", "ezStringView GetShrunk(ezUInt32 uiShrinkCharsFront, ezUInt32 uiShrinkCharsBack = 0) const", asMETHOD(ezStringView, GetShrunk), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringView", "ezStringView GetSubString(ezUInt32 uiFirstCharacter, ezUInt32 uiNumCharacters) const", asMETHOD(ezStringView, GetSubString), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringView", "void ChopAwayFirstCharacterUtf8()", asMETHOD(ezStringView, ChopAwayFirstCharacterUtf8), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringView", "void ChopAwayFirstCharacterAscii()", asMETHOD(ezStringView, ChopAwayFirstCharacterAscii), asCALL_THISCALL));
  // Trim
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringView", "bool TrimWordStart(ezStringView sWord)", asMETHOD(ezStringView, TrimWordStart), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringView", "bool TrimWordEnd(ezStringView sWord)", asMETHOD(ezStringView, TrimWordEnd), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringView", "bool opEquals(ezStringView) const", asFUNCTIONPR(operator==, (ezStringView, ezStringView), bool), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringView", "int opCmp(ezStringView) const", asFUNCTIONPR(ezStringView_opCmp, (ezStringView, ezStringView), int), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringView", "void opAssign(const ezString& in)", asFUNCTION(ezStringView_opAssignString), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezStringView", asBEHAVE_CONSTRUCT, "void f(const ezString& in)", asFUNCTION(ezStringView_ConstructString), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringView", "void opAssign(const ezStringBuilder& in)", asFUNCTION(ezStringView_opAssignStringBuilder), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezStringView", asBEHAVE_CONSTRUCT, "void f(const ezStringBuilder& in)", asFUNCTION(ezStringView_ConstructStringBuilder), asCALL_CDECL_OBJFIRST));
}


//////////////////////////////////////////////////////////////////////////
// ezString
//////////////////////////////////////////////////////////////////////////

static void ezString_Construct(void* memory)
{
  new (memory) ezString();
}

static void ezString_Destruct(void* memory)
{
  ezString* p = (ezString*)memory;
  p->~ezString();
}

static void ezString_ConstructView(void* memory, ezStringView rhs)
{
  new (memory) ezString(rhs);
}

static void ezString_ConstructString(void* memory, const ezString& rhs)
{
  new (memory) ezString(rhs);
}

static void ezString_ConstructStringBuilder(void* memory, const ezStringBuilder& rhs)
{
  new (memory) ezString(rhs);
}

static int ezString_opCmp(const ezString& lhs, const ezString& rhs)
{
  if (lhs < rhs)
    return -1;
  if (rhs < lhs)
    return +1;

  return 0;
}

static void ezString_opAssignStdStr(ezString* lhs, const std::string& rhs)
{
  *lhs = rhs.c_str();
}
static void ezString_opAssignString(ezString* lhs, const ezString& rhs)
{
  *lhs = rhs;
}

static void ezString_opAssignStringView(ezString* lhs, ezStringView rhs)
{
  *lhs = rhs;
}

static void ezString_opAssignStringBuilder(ezString* lhs, const ezStringBuilder& rhs)
{
  *lhs = rhs;
}

static void StdString_opAssignString(std::string* lhs, const ezString& rhs)
{
  lhs->assign(rhs.GetData(), rhs.GetElementCount());
}

void ezAngelScriptEngineSingleton::Register_String()
{
  RegisterStringBase<ezString>(m_pEngine, "ezString");

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezString", "ezStringView GetView() const", asMETHOD(ezString, GetView), asCALL_THISCALL));
  // AS_CHECK(m_pEngine->RegisterObjectMethod("ezString", "uint32 GetCharacterCount() const", asMETHOD(ezString, GetCharacterCount), asCALL_THISCALL));

  // TODO AngelScript: string equals operator
  // AS_CHECK(m_pEngine->RegisterObjectMethod("ezString", "bool opEquals(const ezString& in) const", asFUNCTIONPR(operator==, (const ezString&, const ezString&), bool), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezString", "int opCmp(const ezString& in) const", asFUNCTIONPR(ezString_opCmp, (const ezString&, const ezString&), int), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezString", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ezString_Construct), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezString", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(ezString_Destruct), asCALL_CDECL_OBJFIRST));

  // AS_CHECK(m_pEngine->RegisterObjectMethod("ezString", "void opAssign(const string& in)", asFUNCTION(ezString_opAssignStr), asCALL_CDECL_OBJFIRST));
  // AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezString", asBEHAVE_CONSTRUCT, "void f(const string& in)", asFUNCTION(ezString_ConstructStdStr), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezString", "void opAssign(const ezString& in)", asFUNCTION(ezString_opAssignString), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezString", asBEHAVE_CONSTRUCT, "void f(const ezString& in)", asFUNCTION(ezString_ConstructString), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezString", "void opAssign(const ezStringView)", asFUNCTION(ezString_opAssignStringView), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezString", asBEHAVE_CONSTRUCT, "void f(const ezStringView)", asFUNCTION(ezString_ConstructView), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezString", "void opAssign(const ezStringBuilder& in)", asFUNCTION(ezString_opAssignStringBuilder), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezString", asBEHAVE_CONSTRUCT, "void f(const ezStringBuilder& in)", asFUNCTION(ezString_ConstructStringBuilder), asCALL_CDECL_OBJFIRST));

  // AS_CHECK(m_pEngine->RegisterObjectMethod("string", "void opAssign(const ezString& in)", asFUNCTION(StdString_opAssignString), asCALL_CDECL_OBJFIRST));
  // AS_CHECK(m_pEngine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f(const ezString& in)", asFUNCTION(StdString_ConstructString), asCALL_CDECL_OBJFIRST));
}

//////////////////////////////////////////////////////////////////////////
// ezStringBuilder
//////////////////////////////////////////////////////////////////////////

static void ezStringBuilder_Construct(void* memory)
{
  new (memory) ezStringBuilder();
}

static void ezStringBuilder_ConstructSV1(void* memory, const ezStringView sb)
{
  new (memory) ezStringBuilder(sb);
}

static void ezStringBuilder_ConstructSV4(void* memory, const ezStringView sv1, const ezStringView sv2, const ezStringView sv3, const ezStringView sv4)
{
  new (memory) ezStringBuilder(sv1, sv2, sv3, sv4);
}

static void ezStringBuilder_Destruct(void* memory)
{
  ezStringBuilder* p = (ezStringBuilder*)memory;
  p->~ezStringBuilder();
}

void ezAngelScriptEngineSingleton::Register_StringBuilder()
{
  RegisterStringBase<ezStringBuilder>(m_pEngine, "ezStringBuilder");

  // Constructors
  {
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezStringBuilder", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(ezStringBuilder_Destruct), asCALL_CDECL_OBJFIRST));

    AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezStringBuilder", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ezStringBuilder_Construct), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezStringBuilder", asBEHAVE_CONSTRUCT, "void f(const ezStringView s1)", asFUNCTION(ezStringBuilder_ConstructSV1), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezStringBuilder", asBEHAVE_CONSTRUCT, "void f(const ezStringView s1, const ezStringView s2, const ezStringView s3 = \"\", const ezStringView s4 = \"\")", asFUNCTION(ezStringBuilder_ConstructSV4), asCALL_CDECL_OBJFIRST));
  }

  // Operators
  {
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void opAssign(const ezStringBuilder& in rhs)", asMETHODPR(ezStringBuilder, operator=, (const ezStringBuilder&), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void opAssign(ezStringView rhs)", asMETHODPR(ezStringBuilder, operator=, (ezStringView), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void opAssign(const ezString& in rhs)", asMETHODPR(ezStringBuilder, operator=, (const ezString&), void), asCALL_THISCALL));
  }

  // Methods
  {
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "ezStringView GetView() const", asMETHOD(ezStringBuilder, GetView), asCALL_THISCALL));

    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void Clear()", asMETHOD(ezStringBuilder, Clear), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "ezUInt32 GetCharacterCount() const", asMETHOD(ezStringBuilder, GetCharacterCount), asCALL_THISCALL));

    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void ToUpper()", asMETHOD(ezStringBuilder, ToUpper), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void ToLower()", asMETHOD(ezStringBuilder, ToLower), asCALL_THISCALL));

    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void Set(ezStringView sData1)", asMETHODPR(ezStringBuilder, Set, (ezStringView), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void Set(ezStringView sData1, ezStringView sData2)", asMETHODPR(ezStringBuilder, Set, (ezStringView, ezStringView), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void Set(ezStringView sData1, ezStringView sData2, ezStringView sData3)", asMETHODPR(ezStringBuilder, Set, (ezStringView, ezStringView, ezStringView), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void Set(ezStringView sData1, ezStringView sData2, ezStringView sData3, ezStringView sData4)", asMETHODPR(ezStringBuilder, Set, (ezStringView, ezStringView, ezStringView, ezStringView), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void Set(ezStringView sData1, ezStringView sData2, ezStringView sData3, ezStringView sData4, ezStringView sData5, ezStringView sData6 = \"\")", asMETHODPR(ezStringBuilder, Set, (ezStringView, ezStringView, ezStringView, ezStringView, ezStringView, ezStringView), void), asCALL_THISCALL));

    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void SetPath(ezStringView sData1, ezStringView sData2, ezStringView sData3 = \"\", ezStringView sData4 = \"\")", asMETHOD(ezStringBuilder, SetPath), asCALL_THISCALL));

    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void Append(ezStringView sData1)", asMETHODPR(ezStringBuilder, Append, (ezStringView), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void Append(ezStringView sData1, ezStringView sData2)", asMETHODPR(ezStringBuilder, Append, (ezStringView, ezStringView), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void Append(ezStringView sData1, ezStringView sData2, ezStringView sData3)", asMETHODPR(ezStringBuilder, Append, (ezStringView, ezStringView, ezStringView), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void Append(ezStringView sData1, ezStringView sData2, ezStringView sData3, ezStringView sData4)", asMETHODPR(ezStringBuilder, Append, (ezStringView, ezStringView, ezStringView, ezStringView), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void Append(ezStringView sData1, ezStringView sData2, ezStringView sData3, ezStringView sData4, ezStringView sData5, ezStringView sData6 = \"\")", asMETHODPR(ezStringBuilder, Append, (ezStringView, ezStringView, ezStringView, ezStringView, ezStringView, ezStringView), void), asCALL_THISCALL));

    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void Prepend(ezStringView sData1, ezStringView sData2 = \"\", ezStringView sData3 = \"\", ezStringView sData4 = \"\", ezStringView sData5 = \"\", ezStringView sData6 = \"\")", asMETHODPR(ezStringBuilder, Prepend, (ezStringView, ezStringView, ezStringView, ezStringView, ezStringView, ezStringView), void), asCALL_THISCALL));

    // TODO AngelScript: ezStringBuilder::SetFormat
    // TODO AngelScript: ezStringBuilder::AppendFormat
    // TODO AngelScript: ezStringBuilder::PrependFormat

    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void Shrink(ezUInt32 uiShrinkCharsFront, ezUInt32 uiShrinkCharsBack)", asMETHOD(ezStringBuilder, Shrink), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void Reserve(ezUInt32 uiNumElements)", asMETHOD(ezStringBuilder, Reserve), asCALL_THISCALL));

    // TODO AngelScript: ezStringBuilder::ReplaceFirst
    // TODO AngelScript: ezStringBuilder::ReplaceLast

    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "ezUInt32 ReplaceAll(ezStringView sSearchFor, ezStringView sReplacement)", asMETHOD(ezStringBuilder, ReplaceAll), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "ezUInt32 ReplaceAll_NoCase(ezStringView sSearchFor, ezStringView sReplacement)", asMETHOD(ezStringBuilder, ReplaceAll_NoCase), asCALL_THISCALL));

    // TODO AngelScript: ezStringBuilder::ReplaceWholeWord
    // TODO AngelScript: ezStringBuilder::ReplaceWholeWordAll

    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void MakeCleanPath()", asMETHOD(ezStringBuilder, MakeCleanPath), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void PathParentDirectory(ezUInt32 uiLevelsUp = 1)", asMETHOD(ezStringBuilder, PathParentDirectory), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void AppendPath(ezStringView sPath1, ezStringView sPath2 = \"\", ezStringView sPath3 = \"\", ezStringView sPath4 = \"\")", asMETHOD(ezStringBuilder, AppendPath), asCALL_THISCALL));

    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void AppendWithSeparator(ezStringView sSeparator, ezStringView sData1, ezStringView sData2 = \"\", ezStringView sData3 = \"\", ezStringView sData4 = \"\", ezStringView sData5 = \"\", ezStringView sData6 = \"\")", asMETHOD(ezStringBuilder, AppendWithSeparator), asCALL_THISCALL));

    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void ChangeFileName(ezStringView sNewFileName)", asMETHOD(ezStringBuilder, ChangeFileName), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void ChangeFileNameAndExtension(ezStringView sNewFileNameWithExtension)", asMETHOD(ezStringBuilder, ChangeFileNameAndExtension), asCALL_THISCALL));

    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void ChangeFileExtension(ezStringView sNewExtension, bool bFullExtension = false)", asMETHOD(ezStringBuilder, ChangeFileExtension), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void RemoveFileExtension(bool bFullExtension = false)", asMETHOD(ezStringBuilder, RemoveFileExtension), asCALL_THISCALL));

    // TODO AngelScript: ezStringBuilder::MakeRelativeTo

    // bool IsPathBelowFolder(const char* szPathToFolder)
    // void Trim(const char* szTrimChars = " \f\n\r\t\v")
    // TrimLeft, TrimRight

    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "bool TrimWordStart(ezStringView sWord)", asMETHOD(ezStringBuilder, TrimWordStart), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "bool TrimWordEnd(ezStringView sWord)", asMETHOD(ezStringBuilder, TrimWordEnd), asCALL_THISCALL));
  }

  // TODO AngelScript: Register ezStringBuilder
}

//////////////////////////////////////////////////////////////////////////
// ezTempHashedString
//////////////////////////////////////////////////////////////////////////

static void ezTempHashedString_Construct(void* memory)
{
  new (memory) ezTempHashedString();
}

static void ezTempHashedString_ConstructView(void* memory, ezStringView view)
{
  new (memory) ezTempHashedString(view);
}

static void ezTempHashedString_ConstructHS(void* memory, const ezHashedString& hs)
{
  new (memory) ezTempHashedString(hs);
}

static void ezTempHashedString_AssignStringView(ezTempHashedString* pStr, ezStringView view)
{
  *pStr = view;
}

static void ezTempHashedString_AssignHS(ezTempHashedString* pStr, const ezHashedString& hs)
{
  *pStr = hs;
}

void ezAngelScriptEngineSingleton::Register_TempHashedString()
{
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezTempHashedString", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ezTempHashedString_Construct), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezTempHashedString", asBEHAVE_CONSTRUCT, "void f(const ezStringView)", asFUNCTION(ezTempHashedString_ConstructView), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezTempHashedString", asBEHAVE_CONSTRUCT, "void f(const ezHashedString& in)", asFUNCTION(ezTempHashedString_ConstructHS), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTempHashedString", "void opAssign(ezStringView)", asFUNCTION(ezTempHashedString_AssignStringView), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTempHashedString", "void opAssign(const ezHashedString& in)", asFUNCTION(ezTempHashedString_AssignHS), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTempHashedString", "bool opEquals(ezTempHashedString) const", asMETHODPR(ezTempHashedString, operator==, (const ezTempHashedString&) const, bool), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTempHashedString", "bool IsEmpty() const", asMETHOD(ezTempHashedString, IsEmpty), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTempHashedString", "void Clear()", asMETHOD(ezTempHashedString, Clear), asCALL_THISCALL));
}

//////////////////////////////////////////////////////////////////////////
// ezHashedString
//////////////////////////////////////////////////////////////////////////

static void ezHashedString_Construct(void* memory)
{
  new (memory) ezHashedString();
}

static void ezHashedString_ConstructView(void* memory, ezStringView view)
{
  ezHashedString* obj = new (memory) ezHashedString();
  obj->Assign(view);
}

static void ezHashedString_ConstructHS(void* memory, const ezHashedString& hs)
{
  new (memory) ezHashedString(hs);
}

static void ezHashedString_AssignStringView(ezHashedString* pStr, ezStringView view)
{
  pStr->Assign(view);
}

static bool ezHashedString_EqualsStringView(ezHashedString* pStr, ezStringView view)
{
  return *pStr == view;
}

void ezAngelScriptEngineSingleton::Register_HashedString()
{
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezHashedString", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ezHashedString_Construct), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezHashedString", asBEHAVE_CONSTRUCT, "void f(const ezStringView)", asFUNCTION(ezHashedString_ConstructView), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezHashedString", asBEHAVE_CONSTRUCT, "void f(const ezHashedString& in)", asFUNCTION(ezHashedString_ConstructHS), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezHashedString", "void opAssign(ezStringView)", asFUNCTION(ezHashedString_AssignStringView), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezHashedString", "bool IsEmpty() const", asMETHOD(ezHashedString, IsEmpty), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezHashedString", "void Clear()", asMETHOD(ezHashedString, Clear), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezHashedString", "void Assign(ezStringView)", asMETHODPR(ezHashedString, Assign, (ezStringView), void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezHashedString", "bool opEquals(const ezHashedString& in) const", asMETHODPR(ezHashedString, operator==, (const ezHashedString&) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezHashedString", "bool opEquals(const ezTempHashedString& in) const", asMETHODPR(ezHashedString, operator==, (const ezTempHashedString&) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezHashedString", "bool opEquals(ezStringView) const", asFUNCTION(ezHashedString_EqualsStringView), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezHashedString", "ezStringView GetView() const", asMETHOD(ezHashedString, GetView), asCALL_THISCALL));
}

//////////////////////////////////////////////////////////////////////////
// ezColor
//////////////////////////////////////////////////////////////////////////

static void ezColor_ConstructRGBA(void* memory, float r, float g, float b, float a)
{
  new (memory) ezColor(r, g, b, a);
}

static void ezColor_ConstructGamma(void* memory, const ezColorGammaUB& col)
{
  new (memory) ezColor(col);
}

void ezAngelScriptEngineSingleton::Register_Color()
{
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezColor", "float r", asOFFSET(ezColor, r)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezColor", "float g", asOFFSET(ezColor, g)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezColor", "float b", asOFFSET(ezColor, b)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezColor", "float a", asOFFSET(ezColor, a)));

  // static functions
  {
    m_pEngine->SetDefaultNamespace("ezColor");

    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor AliceBlue", (void*)&ezColor::AliceBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor AntiqueWhite", (void*)&ezColor::AntiqueWhite));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Aqua", (void*)&ezColor::Aqua));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Aquamarine", (void*)&ezColor::Aquamarine));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Azure", (void*)&ezColor::Azure));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Beige", (void*)&ezColor::Beige));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Bisque", (void*)&ezColor::Bisque));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Black", (void*)&ezColor::Black));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor BlanchedAlmond", (void*)&ezColor::BlanchedAlmond));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Blue", (void*)&ezColor::Blue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor BlueViolet", (void*)&ezColor::BlueViolet));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Brown", (void*)&ezColor::Brown));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor BurlyWood", (void*)&ezColor::BurlyWood));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor CadetBlue", (void*)&ezColor::CadetBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Chartreuse", (void*)&ezColor::Chartreuse));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Chocolate", (void*)&ezColor::Chocolate));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Coral", (void*)&ezColor::Coral));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor CornflowerBlue", (void*)&ezColor::CornflowerBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Cornsilk", (void*)&ezColor::Cornsilk));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Crimson", (void*)&ezColor::Crimson));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Cyan", (void*)&ezColor::Cyan));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DarkBlue", (void*)&ezColor::DarkBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DarkCyan", (void*)&ezColor::DarkCyan));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DarkGoldenRod", (void*)&ezColor::DarkGoldenRod));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DarkGray", (void*)&ezColor::DarkGray));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DarkGrey", (void*)&ezColor::DarkGrey));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DarkGreen", (void*)&ezColor::DarkGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DarkKhaki", (void*)&ezColor::DarkKhaki));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DarkMagenta", (void*)&ezColor::DarkMagenta));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DarkOliveGreen", (void*)&ezColor::DarkOliveGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DarkOrange", (void*)&ezColor::DarkOrange));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DarkOrchid", (void*)&ezColor::DarkOrchid));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DarkRed", (void*)&ezColor::DarkRed));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DarkSalmon", (void*)&ezColor::DarkSalmon));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DarkSeaGreen", (void*)&ezColor::DarkSeaGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DarkSlateBlue", (void*)&ezColor::DarkSlateBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DarkSlateGray", (void*)&ezColor::DarkSlateGray));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DarkSlateGrey", (void*)&ezColor::DarkSlateGrey));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DarkTurquoise", (void*)&ezColor::DarkTurquoise));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DarkViolet", (void*)&ezColor::DarkViolet));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DeepPink", (void*)&ezColor::DeepPink));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DeepSkyBlue", (void*)&ezColor::DeepSkyBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DimGray", (void*)&ezColor::DimGray));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DimGrey", (void*)&ezColor::DimGrey));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor DodgerBlue", (void*)&ezColor::DodgerBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor FireBrick", (void*)&ezColor::FireBrick));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor FloralWhite", (void*)&ezColor::FloralWhite));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor ForestGreen", (void*)&ezColor::ForestGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Fuchsia", (void*)&ezColor::Fuchsia));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Gainsboro", (void*)&ezColor::Gainsboro));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor GhostWhite", (void*)&ezColor::GhostWhite));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Gold", (void*)&ezColor::Gold));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor GoldenRod", (void*)&ezColor::GoldenRod));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Gray", (void*)&ezColor::Gray));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Grey", (void*)&ezColor::Grey));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Green", (void*)&ezColor::Green));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor GreenYellow", (void*)&ezColor::GreenYellow));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor HoneyDew", (void*)&ezColor::HoneyDew));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor HotPink", (void*)&ezColor::HotPink));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor IndianRed", (void*)&ezColor::IndianRed));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Indigo", (void*)&ezColor::Indigo));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Ivory", (void*)&ezColor::Ivory));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Khaki", (void*)&ezColor::Khaki));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Lavender", (void*)&ezColor::Lavender));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor LavenderBlush", (void*)&ezColor::LavenderBlush));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor LawnGreen", (void*)&ezColor::LawnGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor LemonChiffon", (void*)&ezColor::LemonChiffon));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor LightBlue", (void*)&ezColor::LightBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor LightCoral", (void*)&ezColor::LightCoral));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor LightCyan", (void*)&ezColor::LightCyan));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor LightGoldenRodYellow", (void*)&ezColor::LightGoldenRodYellow));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor LightGray", (void*)&ezColor::LightGray));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor LightGrey", (void*)&ezColor::LightGrey));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor LightGreen", (void*)&ezColor::LightGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor LightPink", (void*)&ezColor::LightPink));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor LightSalmon", (void*)&ezColor::LightSalmon));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor LightSeaGreen", (void*)&ezColor::LightSeaGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor LightSkyBlue", (void*)&ezColor::LightSkyBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor LightSlateGray", (void*)&ezColor::LightSlateGray));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor LightSlateGrey", (void*)&ezColor::LightSlateGrey));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor LightSteelBlue", (void*)&ezColor::LightSteelBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor LightYellow", (void*)&ezColor::LightYellow));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Lime", (void*)&ezColor::Lime));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor LimeGreen", (void*)&ezColor::LimeGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Linen", (void*)&ezColor::Linen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Magenta", (void*)&ezColor::Magenta));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Maroon", (void*)&ezColor::Maroon));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor MediumAquaMarine", (void*)&ezColor::MediumAquaMarine));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor MediumBlue", (void*)&ezColor::MediumBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor MediumOrchid", (void*)&ezColor::MediumOrchid));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor MediumPurple", (void*)&ezColor::MediumPurple));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor MediumSeaGreen", (void*)&ezColor::MediumSeaGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor MediumSlateBlue", (void*)&ezColor::MediumSlateBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor MediumSpringGreen", (void*)&ezColor::MediumSpringGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor MediumTurquoise", (void*)&ezColor::MediumTurquoise));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor MediumVioletRed", (void*)&ezColor::MediumVioletRed));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor MidnightBlue", (void*)&ezColor::MidnightBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor MintCream", (void*)&ezColor::MintCream));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor MistyRose", (void*)&ezColor::MistyRose));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Moccasin", (void*)&ezColor::Moccasin));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor NavajoWhite", (void*)&ezColor::NavajoWhite));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Navy", (void*)&ezColor::Navy));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor OldLace", (void*)&ezColor::OldLace));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Olive", (void*)&ezColor::Olive));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor OliveDrab", (void*)&ezColor::OliveDrab));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Orange", (void*)&ezColor::Orange));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor OrangeRed", (void*)&ezColor::OrangeRed));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Orchid", (void*)&ezColor::Orchid));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor PaleGoldenRod", (void*)&ezColor::PaleGoldenRod));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor PaleGreen", (void*)&ezColor::PaleGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor PaleTurquoise", (void*)&ezColor::PaleTurquoise));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor PaleVioletRed", (void*)&ezColor::PaleVioletRed));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor PapayaWhip", (void*)&ezColor::PapayaWhip));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor PeachPuff", (void*)&ezColor::PeachPuff));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Peru", (void*)&ezColor::Peru));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Pink", (void*)&ezColor::Pink));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Plum", (void*)&ezColor::Plum));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor PowderBlue", (void*)&ezColor::PowderBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Purple", (void*)&ezColor::Purple));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor RebeccaPurple", (void*)&ezColor::RebeccaPurple));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Red", (void*)&ezColor::Red));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor RosyBrown", (void*)&ezColor::RosyBrown));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor RoyalBlue", (void*)&ezColor::RoyalBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor SaddleBrown", (void*)&ezColor::SaddleBrown));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Salmon", (void*)&ezColor::Salmon));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor SandyBrown", (void*)&ezColor::SandyBrown));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor SeaGreen", (void*)&ezColor::SeaGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor SeaShell", (void*)&ezColor::SeaShell));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Sienna", (void*)&ezColor::Sienna));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Silver", (void*)&ezColor::Silver));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor SkyBlue", (void*)&ezColor::SkyBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor SlateBlue", (void*)&ezColor::SlateBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor SlateGray", (void*)&ezColor::SlateGray));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor SlateGrey", (void*)&ezColor::SlateGrey));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Snow", (void*)&ezColor::Snow));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor SpringGreen", (void*)&ezColor::SpringGreen));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor SteelBlue", (void*)&ezColor::SteelBlue));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Tan", (void*)&ezColor::Tan));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Teal", (void*)&ezColor::Teal));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Thistle", (void*)&ezColor::Thistle));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Tomato", (void*)&ezColor::Tomato));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Turquoise", (void*)&ezColor::Turquoise));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Violet", (void*)&ezColor::Violet));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Wheat", (void*)&ezColor::Wheat));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor White", (void*)&ezColor::White));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor WhiteSmoke", (void*)&ezColor::WhiteSmoke));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor Yellow", (void*)&ezColor::Yellow));
    AS_CHECK(m_pEngine->RegisterGlobalProperty("const ezColor YellowGreen", (void*)&ezColor::YellowGreen));

    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezColor MakeNaN()", asFUNCTION(ezColor::MakeNaN), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezColor MakeZero()", asFUNCTION(ezColor::MakeZero), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezColor MakeRGBA(float r, float g, float b, float a)", asFUNCTION(ezColor::MakeRGBA), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezColor MakeFromKelvin(ezUInt32 uiKelvin)", asFUNCTION(ezColor::MakeFromKelvin), asCALL_CDECL));
    AS_CHECK(m_pEngine->RegisterGlobalFunction("ezColor MakeHSV(float fHue, float fSat, float fVal)", asFUNCTION(ezColor::MakeHSV), asCALL_CDECL));

    m_pEngine->SetDefaultNamespace("");
  }

  // Constructors
  {
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezColor", asBEHAVE_CONSTRUCT, "void f(float r, float g, float b, float a = 1.0f)", asFUNCTION(ezColor_ConstructRGBA), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezColor", asBEHAVE_CONSTRUCT, "void f(const ezColorGammaUB& in)", asFUNCTION(ezColor_ConstructGamma), asCALL_CDECL_OBJFIRST));
  }

  // Operators
  {
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "void opAssign(const ezColorGammaUB& in)", asMETHODPR(ezColor, operator=, (const ezColorGammaUB&), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "void opAddAssign(const ezColor& in)", asMETHODPR(ezColor, operator+=, (const ezColor&), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "void opSubAssign(const ezColor& in)", asMETHODPR(ezColor, operator-=, (const ezColor&), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "void opMulAssign(const ezColor& in)", asMETHODPR(ezColor, operator*=, (const ezColor&), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "void opMulAssign(float)", asMETHODPR(ezColor, operator*=, (float), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "void opDivAssign(float)", asMETHODPR(ezColor, operator/=, (float), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "void opMulAssign(const ezMat4& in)", asMETHODPR(ezColor, operator*=, (const ezMat4&), void), asCALL_THISCALL));


    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "ezColor opAdd(const ezColor& in) const", asFUNCTIONPR(operator+, (const ezColor&, const ezColor&), const ezColor), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "ezColor opSub(const ezColor& in) const", asFUNCTIONPR(operator-, (const ezColor&, const ezColor&), const ezColor), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "ezColor opMul(const ezColor& in) const", asFUNCTIONPR(operator*, (const ezColor&, const ezColor&), const ezColor), asCALL_CDECL_OBJFIRST));

    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "ezColor opMul(float) const", asFUNCTIONPR(operator*, (const ezColor&, float), const ezColor), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "ezColor opMul_r(float) const", asFUNCTIONPR(operator*, (float, const ezColor&), const ezColor), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "ezColor opDiv(float) const", asFUNCTIONPR(operator/, (const ezColor&, float), const ezColor), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "ezColor opMul_r(const ezMat4& in) const", asFUNCTIONPR(operator*, (const ezMat4&, const ezColor&), const ezColor), asCALL_CDECL_OBJFIRST));

    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "bool opEquals(const ezColor& in) const", asFUNCTIONPR(operator==, (const ezColor&, const ezColor&), bool), asCALL_CDECL_OBJFIRST));
  }

  // Methods
  {
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "void SetRGB(float r, float g, float b)", asMETHOD(ezColor, SetRGB), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "void SetRGBA(float r, float g, float b, float a = 1.0f)", asMETHOD(ezColor, SetRGBA), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "void GetHSV(float& out fHue, float& out fSaturation, float& out fValue) const", asMETHOD(ezColor, GetHSV), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "ezVec4 GetAsVec4() const", asMETHOD(ezColor, GetAsVec4), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "bool IsNormalized() const", asMETHOD(ezColor, IsNormalized), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "float CalcAverageRGB() const", asMETHOD(ezColor, CalcAverageRGB), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "float GetSaturation() const", asMETHOD(ezColor, GetSaturation), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "float GetLuminance() const", asMETHOD(ezColor, GetLuminance), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "ezColor GetInvertedColor() const", asMETHOD(ezColor, GetInvertedColor), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "ezColor GetComplementaryColor() const", asMETHOD(ezColor, GetComplementaryColor), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "void ScaleRGB(float)", asMETHOD(ezColor, ScaleRGB), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "void ScaleRGBA(float)", asMETHOD(ezColor, ScaleRGBA), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "float ComputeHdrMultiplier() const", asMETHOD(ezColor, ComputeHdrMultiplier), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "float ComputeHdrExposureValue() const", asMETHOD(ezColor, ComputeHdrExposureValue), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "void ApplyHdrExposureValue(float fExposure)", asMETHOD(ezColor, ApplyHdrExposureValue), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "void NormalizeToLdrRange()", asMETHOD(ezColor, NormalizeToLdrRange), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "ezColor GetDarker(float fFactor = 2.0f) const", asMETHOD(ezColor, GetDarker), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "bool IsNaN() const", asMETHOD(ezColor, IsNaN), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "bool IsValid() const", asMETHOD(ezColor, IsValid), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "bool IsIdenticalRGB(const ezColor& in) const", asMETHOD(ezColor, IsIdenticalRGB), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "bool IsIdenticalRGBA(const ezColor& in) const", asMETHOD(ezColor, IsIdenticalRGBA), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "bool IsEqualRGB(const ezColor& in, float fEpsilon) const", asMETHOD(ezColor, IsEqualRGB), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "bool IsEqualRGBA(const ezColor& in, float fEpsilon) const", asMETHOD(ezColor, IsEqualRGBA), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "ezColor WithAlpha(float fAlpha) const", asMETHOD(ezColor, WithAlpha), asCALL_THISCALL));
  }
}

//////////////////////////////////////////////////////////////////////////
// ezColorGammaUB
//////////////////////////////////////////////////////////////////////////

void ezColorGamma_ConstructRGBA(void* memory, ezUInt8 r, ezUInt8 g, ezUInt8 b, ezUInt8 a)
{
  new (memory) ezColorGammaUB(r, g, b, a);
}

void ezColorGamma_ConstructColor(void* memory, const ezColor& col)
{
  new (memory) ezColorGammaUB(col);
}

void ezAngelScriptEngineSingleton::Register_ColorGammaUB()
{
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezColorGammaUB", "uint8 r", asOFFSET(ezColorGammaUB, r)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezColorGammaUB", "uint8 g", asOFFSET(ezColorGammaUB, g)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezColorGammaUB", "uint8 b", asOFFSET(ezColorGammaUB, b)));
  AS_CHECK(m_pEngine->RegisterObjectProperty("ezColorGammaUB", "uint8 a", asOFFSET(ezColorGammaUB, a)));

  // Constructors
  {
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezColorGammaUB", asBEHAVE_CONSTRUCT, "void f(uint8 r, uint8 g, uint8 b, uint8 a = 255)", asFUNCTION(ezColorGamma_ConstructRGBA), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezColorGammaUB", asBEHAVE_CONSTRUCT, "void f(const ezColor& in)", asFUNCTION(ezColorGamma_ConstructColor), asCALL_CDECL_OBJFIRST));
  }

  // Operators
  {
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColorGammaUB", "void opAssign(const ezColor& in)", asMETHODPR(ezColorGammaUB, operator=, (const ezColor&), void), asCALL_THISCALL));
  }

  // Methods
  {
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColorGammaUB", "ezColor ToLinearFloat() const", asMETHOD(ezColorGammaUB, ToLinearFloat), asCALL_THISCALL));
  }
}


//////////////////////////////////////////////////////////////////////////
// ezRandom
//////////////////////////////////////////////////////////////////////////

void ezAngelScriptEngineSingleton::Register_Random()
{
  // Methods
  {
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezRandom", "uint32 UInt()", asMETHOD(ezRandom, UInt), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezRandom", "uint32 UIntInRange(uint32 uiRange)", asMETHOD(ezRandom, UIntInRange), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezRandom", "uint32 UInt32Index(ezUInt32 uiArraySize, ezUInt32 uiFallbackValue = 0xFFFFFFFF)", asMETHOD(ezRandom, UInt32Index), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezRandom", "uint16 UInt16Index(ezUInt16 uiArraySize, ezUInt16 uiFallbackValue = 0xFFFF)", asMETHOD(ezRandom, UInt16Index), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezRandom", "int32 IntMinMax(ezInt32 iMinValue, ezInt32 iMaxValue)", asMETHOD(ezRandom, IntMinMax), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezRandom", "bool Bool()", asMETHOD(ezRandom, Bool), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezRandom", "double DoubleZeroToOneExclusive()", asMETHOD(ezRandom, DoubleZeroToOneExclusive), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezRandom", "double DoubleZeroToOneInclusive()", asMETHOD(ezRandom, DoubleZeroToOneInclusive), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezRandom", "double DoubleMinMax(double fMinValue, double fMaxValue)", asMETHOD(ezRandom, DoubleMinMax), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezRandom", "double DoubleVariance(double fValue, double fVariance)", asMETHOD(ezRandom, DoubleVariance), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezRandom", "double DoubleVarianceAroundZero(double fAbsMaxValue)", asMETHOD(ezRandom, DoubleVarianceAroundZero), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezRandom", "float FloatZeroToOneExclusive()", asMETHOD(ezRandom, FloatZeroToOneExclusive), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezRandom", "float FloatZeroToOneInclusive()", asMETHOD(ezRandom, FloatZeroToOneInclusive), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezRandom", "float FloatMinMax(float fMinValue, float fMaxValue)", asMETHOD(ezRandom, FloatMinMax), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezRandom", "float FloatVariance(float fValue, float fVariance)", asMETHOD(ezRandom, FloatVariance), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezRandom", "float FloatVarianceAroundZero(float fAbsMaxValue)", asMETHOD(ezRandom, FloatVarianceAroundZero), asCALL_THISCALL));
  }
}

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
