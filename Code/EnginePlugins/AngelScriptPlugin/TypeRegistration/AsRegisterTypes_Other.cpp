#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <Core/World/SpatialData.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Time/Clock.h>

//////////////////////////////////////////////////////////////////////////
// ezRTTI
//////////////////////////////////////////////////////////////////////////

const ezRTTI* ezRTTI_GetType(ezStringView sName)
{
  return ezRTTI::FindTypeByName(sName);
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

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "ezTime opMul(double) const", asFUNCTIONPR(operator*, (const ezTime&, double), ezTime), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "ezTime opMul_r(double) const", asFUNCTIONPR(operator*, (double, const ezTime&), ezTime), asCALL_CDECL_OBJLAST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "ezTime opMul(const ezTime& in) const", asFUNCTIONPR(operator*, (const ezTime&, const ezTime&), ezTime), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "ezTime opDiv(double) const", asFUNCTIONPR(operator/, (const ezTime&, double), ezTime), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "ezTime opDiv_r(double) const", asFUNCTIONPR(operator/, (double, const ezTime&), ezTime), asCALL_CDECL_OBJLAST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezTime", "ezTime opDiv(const ezTime& in) const", asFUNCTIONPR(operator/, (const ezTime&, const ezTime&), ezTime), asCALL_CDECL_OBJFIRST));
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

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezClock", "ezTime GetAccumulatedTime() const", asMETHOD(ezClock, GetAccumulatedTime), asCALL_THISCALL));
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
// ezColor
//////////////////////////////////////////////////////////////////////////

static void ezColor_ConstructRGBA(void* pMemory, float r, float g, float b, float a)
{
  new (pMemory) ezColor(r, g, b, a);
}

static void ezColor_ConstructGamma(void* pMemory, const ezColorGammaUB& col)
{
  new (pMemory) ezColor(col);
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
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "ezColor opMul_r(float) const", asFUNCTIONPR(operator*, (const ezColor&, float), const ezColor), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "ezColor opDiv(float) const", asFUNCTIONPR(operator/, (const ezColor&, float), const ezColor), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezColor", "ezColor opMul_r(const ezMat4& in) const", asFUNCTIONPR(operator*, (const ezMat4&, const ezColor&), const ezColor), asCALL_CDECL_OBJLAST));

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

void ezColorGamma_ConstructRGBA(void* pMemory, ezUInt8 r, ezUInt8 g, ezUInt8 b, ezUInt8 a)
{
  new (pMemory) ezColorGammaUB(r, g, b, a);
}

void ezColorGamma_ConstructColor(void* pMemory, const ezColor& col)
{
  new (pMemory) ezColorGammaUB(col);
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
// ezSpatial
//////////////////////////////////////////////////////////////////////////

void ezSpatial_FindObjectsInSphere(ezStringView sCategory, const ezVec3& vCenter, float fRadius, asIScriptFunction* pCallback)
{
  ezWorld* pWorld = ezAngelScriptUtils::GetThreadLocalWorld();

  auto category = ezSpatialData::FindCategory(sCategory);
  if (category != ezInvalidSpatialDataCategory)
  {
    ezSpatialSystem::QueryParams params;
    params.m_uiCategoryBitmask = category.GetBitmask();

    pWorld->GetSpatialSystem()->FindObjectsInSphere(ezBoundingSphere::MakeFromCenterAndRadius(vCenter, fRadius), params, [pCallback](ezGameObject* go) -> ezVisitorExecution::Enum
      {
        asIScriptContext* pCtx = asGetActiveContext();
        pCtx->PushState();

        pCtx->Prepare(pCallback);
        pCtx->SetArgObject(0, go);
        pCtx->Execute();

        const ezVisitorExecution::Enum res = (pCtx->GetReturnByte() != 0) ? ezVisitorExecution::Continue : ezVisitorExecution::Stop;

        pCtx->PopState();

        return res;
        //
      });
  }

  // need to release the refcount
  pCallback->Release();
}

void ezAngelScriptEngineSingleton::Register_Spatial()
{
  AS_CHECK(m_pEngine->RegisterFuncdef("bool ReportObjectCB(ezGameObject@)"));

  m_pEngine->SetDefaultNamespace("ezSpatial");

  AS_CHECK(m_pEngine->RegisterGlobalFunction("void FindObjectsInSphere(ezStringView sCategory, const ezVec3& in vCenter, float fRadius, ReportObjectCB@ callback)", asFUNCTION(ezSpatial_FindObjectsInSphere), asCALL_CDECL));

  m_pEngine->SetDefaultNamespace("");
}
