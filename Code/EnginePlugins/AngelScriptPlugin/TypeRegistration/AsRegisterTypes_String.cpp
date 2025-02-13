#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <AngelScriptPlugin/Runtime/AsStringFactory.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>

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

static void ezStringView_ConstructView(void* pMemory, const ezStringView rhs)
{
  new (pMemory) ezStringView(rhs);
}

static void ezStringView_ConstructString(void* pMemory, const ezString& rhs)
{
  const ezString& str = ezAsStringFactory::GetFactory()->StoreString(rhs);

  new (pMemory) ezStringView(str);
}

static void ezStringView_opAssignString(ezStringView* lhs, const ezString& rhs)
{
  const ezString& str = ezAsStringFactory::GetFactory()->StoreString(rhs);

  *lhs = str;
}

static void ezStringView_ConstructStringBuilder(void* pMemory, const ezStringBuilder& rhs)
{
  const ezString& str = ezAsStringFactory::GetFactory()->StoreString(rhs);

  new (pMemory) ezStringView(str);
}

static void ezStringView_opAssignStringBuilder(ezStringView* lhs, const ezStringBuilder& rhs)
{
  const ezString& str = ezAsStringFactory::GetFactory()->StoreString(rhs);

  *lhs = str;
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

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezStringView", asBEHAVE_CONSTRUCT, "void f(const ezStringView)", asFUNCTION(ezStringView_ConstructView), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezStringView", asBEHAVE_CONSTRUCT, "void f(const ezString& in)", asFUNCTION(ezStringView_ConstructString), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringView", "void opAssign(const ezString& in)", asFUNCTION(ezStringView_opAssignString), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezStringView", asBEHAVE_CONSTRUCT, "void f(const ezStringBuilder& in)", asFUNCTION(ezStringView_ConstructStringBuilder), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringView", "void opAssign(const ezStringBuilder& in)", asFUNCTION(ezStringView_opAssignStringBuilder), asCALL_CDECL_OBJFIRST));
}


//////////////////////////////////////////////////////////////////////////
// ezString
//////////////////////////////////////////////////////////////////////////

static void ezString_Construct(void* pMemory)
{
  new (pMemory) ezString();
}

static void ezString_Destruct(void* pMemory)
{
  ezString* p = (ezString*)pMemory;
  p->~ezString();
}

static void ezString_ConstructView(void* pMemory, ezStringView rhs)
{
  new (pMemory) ezString(rhs);
}

static void ezString_ConstructString(void* pMemory, const ezString& rhs)
{
  new (pMemory) ezString(rhs);
}

static void ezString_ConstructStringBuilder(void* pMemory, const ezStringBuilder& rhs)
{
  new (pMemory) ezString(rhs);
}

static void ezString_ConstructHS(void* pMemory, const ezHashedString& rhs)
{
  new (pMemory) ezString(rhs.GetView());
}

static int ezString_opCmp(const ezString& lhs, const ezString& rhs)
{
  if (lhs < rhs)
    return -1;
  if (rhs < lhs)
    return +1;

  return 0;
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

static void ezString_opAssignHS(ezString* lhs, const ezHashedString& rhs)
{
  *lhs = rhs.GetView();
}

void ezAngelScriptEngineSingleton::Register_String()
{
  RegisterStringBase<ezString>(m_pEngine, "ezString");

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezString", "ezStringView GetView() const", asMETHOD(ezString, GetView), asCALL_THISCALL));
  // AS_CHECK(m_pEngine->RegisterObjectMethod("ezString", "uint32 GetCharacterCount() const", asMETHOD(ezString, GetCharacterCount), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezString", "int opCmp(const ezString& in) const", asFUNCTIONPR(ezString_opCmp, (const ezString&, const ezString&), int), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezString", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ezString_Construct), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezString", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(ezString_Destruct), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezString", asBEHAVE_CONSTRUCT, "void f(const ezStringView)", asFUNCTION(ezString_ConstructView), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezString", "void opAssign(const ezStringView)", asFUNCTION(ezString_opAssignStringView), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezString", asBEHAVE_CONSTRUCT, "void f(const ezString& in)", asFUNCTION(ezString_ConstructString), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezString", "void opAssign(const ezString& in)", asFUNCTION(ezString_opAssignString), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezString", asBEHAVE_CONSTRUCT, "void f(const ezStringBuilder& in)", asFUNCTION(ezString_ConstructStringBuilder), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezString", "void opAssign(const ezStringBuilder& in)", asFUNCTION(ezString_opAssignStringBuilder), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezString", asBEHAVE_CONSTRUCT, "void f(const ezHashedString& in)", asFUNCTION(ezString_ConstructHS), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezString", "void opAssign(const ezHashedString& in)", asFUNCTION(ezString_opAssignHS), asCALL_CDECL_OBJFIRST));
}

//////////////////////////////////////////////////////////////////////////
// ezStringBuilder
//////////////////////////////////////////////////////////////////////////

static void ezStringBuilder_Construct(void* pMemory)
{
  new (pMemory) ezStringBuilder();
}

static void ezStringBuilder_ConstructSV1(void* pMemory, const ezStringView sView)
{
  new (pMemory) ezStringBuilder(sView);
}

static void ezStringBuilder_ConstructSV4(void* pMemory, const ezStringView sV1, const ezStringView sV2, const ezStringView sV3, const ezStringView sV4)
{
  new (pMemory) ezStringBuilder(sV1, sV2, sV3, sV4);
}

static void ezStringBuilder_Destruct(void* pMemory)
{
  ezStringBuilder* p = (ezStringBuilder*)pMemory;
  p->~ezStringBuilder();
}

static void ezStringBuilder_Format(ezStringBuilder& ref_sStr, asIScriptGeneric* pGen)
{
  const ezUInt32 uiNumArgs = (ezUInt32)pGen->GetArgCount();
  const ezStringView sText = *((ezStringView*)pGen->GetArgObject(0));

  ezHybridArray<ezString, 12> stringStorage;
  ezHybridArray<ezStringView, 12> stringViews;
  stringStorage.Reserve(pGen->GetArgCount() - 1);

  ezVariant res;
  for (ezUInt32 uiArg = 1; uiArg < uiNumArgs; ++uiArg)
  {
    auto argTypeId = pGen->GetArgTypeId(uiArg);

    if (ezAngelScriptUtils::ReadFromAsTypeAtLocation(pGen->GetEngine(), argTypeId, pGen->GetArgAddress(uiArg), res).Succeeded())
    {
      stringStorage.PushBack(res.ConvertTo<ezString>());
      continue;
    }

    const char* typeName = "null";
    if (const asITypeInfo* pInfo = pGen->GetEngine()->GetTypeInfoById(argTypeId))
    {
      typeName = pInfo->GetName();
    }

    ezLog::Error("Call to 'ezStringBuilder::SetFormat': Argument {} got an unsupported type '{}' ({})", uiArg, typeName, argTypeId);
    break;
  }

  stringViews.Reserve(stringStorage.GetCount());
  for (auto& s : stringStorage)
  {
    stringViews.PushBack(s);
  }

  ezFormatString fs(sText);
  fs.BuildFormattedText(ref_sStr, stringViews.GetData(), stringViews.GetCount());
}

static void ezStringBuilder_SetFormat(asIScriptGeneric* pGen)
{
  ezStringBuilder& sb = *((ezStringBuilder*)pGen->GetObject());
  sb.Clear();

  ezStringBuilder_Format(sb, pGen);
}

static void ezStringBuilder_AppendFormat(asIScriptGeneric* pGen)
{
  ezStringBuilder& sb = *((ezStringBuilder*)pGen->GetObject());

  ezStringBuilder tmp;
  ezStringBuilder_Format(tmp, pGen);
  sb.Append(tmp);
}

static void ezStringBuilder_PrependFormat(asIScriptGeneric* pGen)
{
  ezStringBuilder& sb = *((ezStringBuilder*)pGen->GetObject());

  ezStringBuilder tmp;
  ezStringBuilder_Format(tmp, pGen);
  sb.Prepend(tmp);
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

    // SetFormat
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void SetFormat(ezStringView sText, ?&in VarArg1)", asFUNCTION(ezStringBuilder_SetFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void SetFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2)", asFUNCTION(ezStringBuilder_SetFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void SetFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3)", asFUNCTION(ezStringBuilder_SetFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void SetFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4)", asFUNCTION(ezStringBuilder_SetFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void SetFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5)", asFUNCTION(ezStringBuilder_SetFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void SetFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5, ?&in VarArg6)", asFUNCTION(ezStringBuilder_SetFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void SetFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5, ?&in VarArg6, ?&in VarArg7)", asFUNCTION(ezStringBuilder_SetFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void SetFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5, ?&in VarArg6, ?&in VarArg7, ?&in VarArg8)", asFUNCTION(ezStringBuilder_SetFormat), asCALL_GENERIC));

    // AppendFormat
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void AppendFormat(ezStringView sText, ?&in VarArg1)", asFUNCTION(ezStringBuilder_AppendFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void AppendFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2)", asFUNCTION(ezStringBuilder_AppendFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void AppendFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3)", asFUNCTION(ezStringBuilder_AppendFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void AppendFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4)", asFUNCTION(ezStringBuilder_AppendFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void AppendFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5)", asFUNCTION(ezStringBuilder_AppendFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void AppendFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5, ?&in VarArg6)", asFUNCTION(ezStringBuilder_AppendFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void AppendFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5, ?&in VarArg6, ?&in VarArg7)", asFUNCTION(ezStringBuilder_AppendFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void AppendFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5, ?&in VarArg6, ?&in VarArg7, ?&in VarArg8)", asFUNCTION(ezStringBuilder_AppendFormat), asCALL_GENERIC));

    // PrependFormat
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void PrependFormat(ezStringView sText, ?&in VarArg1)", asFUNCTION(ezStringBuilder_PrependFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void PrependFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2)", asFUNCTION(ezStringBuilder_PrependFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void PrependFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3)", asFUNCTION(ezStringBuilder_PrependFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void PrependFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4)", asFUNCTION(ezStringBuilder_PrependFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void PrependFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5)", asFUNCTION(ezStringBuilder_PrependFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void PrependFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5, ?&in VarArg6)", asFUNCTION(ezStringBuilder_PrependFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void PrependFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5, ?&in VarArg6, ?&in VarArg7)", asFUNCTION(ezStringBuilder_PrependFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezStringBuilder", "void PrependFormat(ezStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5, ?&in VarArg6, ?&in VarArg7, ?&in VarArg8)", asFUNCTION(ezStringBuilder_PrependFormat), asCALL_GENERIC));

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
}

//////////////////////////////////////////////////////////////////////////
// ezTempHashedString
//////////////////////////////////////////////////////////////////////////

static void ezTempHashedString_Construct(void* pMemory)
{
  new (pMemory) ezTempHashedString();
}

static void ezTempHashedString_ConstructView(void* pMemory, ezStringView sView)
{
  new (pMemory) ezTempHashedString(sView);
}

static void ezTempHashedString_ConstructTempHashed(void* pMemory, const ezTempHashedString& sString)
{
  new (pMemory) ezTempHashedString(sString);
}

static void ezTempHashedString_ConstructHS(void* pMemory, const ezHashedString& sString)
{
  new (pMemory) ezTempHashedString(sString);
}

static void ezTempHashedString_AssignStringView(ezTempHashedString* pStr, ezStringView sView)
{
  *pStr = sView;
}

static void ezTempHashedString_AssignHS(ezTempHashedString* pStr, const ezHashedString& sString)
{
  *pStr = sString;
}

void ezAngelScriptEngineSingleton::Register_TempHashedString()
{
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezTempHashedString", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ezTempHashedString_Construct), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezTempHashedString", asBEHAVE_CONSTRUCT, "void f(const ezTempHashedString& in)", asFUNCTION(ezTempHashedString_ConstructTempHashed), asCALL_CDECL_OBJFIRST));
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

static void ezHashedString_Construct(void* pMemory)
{
  new (pMemory) ezHashedString();
}

static void ezHashedString_ConstructView(void* pMemory, ezStringView sView)
{
  ezHashedString* obj = new (pMemory) ezHashedString();
  obj->Assign(sView);
}

static void ezHashedString_ConstructHS(void* pMemory, const ezHashedString& sString)
{
  new (pMemory) ezHashedString(sString);
}

static void ezHashedString_AssignStringView(ezHashedString* pStr, const ezStringView sView)
{
  pStr->Assign(sView);
}

static bool ezHashedString_EqualsStringView(ezHashedString* pStr, const ezStringView sView)
{
  return *pStr == sView;
}

void ezAngelScriptEngineSingleton::Register_HashedString()
{
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezHashedString", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ezHashedString_Construct), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezHashedString", asBEHAVE_CONSTRUCT, "void f(const ezStringView)", asFUNCTION(ezHashedString_ConstructView), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezHashedString", asBEHAVE_CONSTRUCT, "void f(const ezHashedString& in)", asFUNCTION(ezHashedString_ConstructHS), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezHashedString", "void opAssign(const ezStringView)", asFUNCTION(ezHashedString_AssignStringView), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezHashedString", "bool IsEmpty() const", asMETHOD(ezHashedString, IsEmpty), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezHashedString", "void Clear()", asMETHOD(ezHashedString, Clear), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezHashedString", "void Assign(const ezStringView)", asMETHODPR(ezHashedString, Assign, (ezStringView), void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezHashedString", "bool opEquals(const ezHashedString& in) const", asMETHODPR(ezHashedString, operator==, (const ezHashedString&) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezHashedString", "bool opEquals(const ezTempHashedString& in) const", asMETHODPR(ezHashedString, operator==, (const ezTempHashedString&) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezHashedString", "bool opEquals(const ezStringView) const", asFUNCTION(ezHashedString_EqualsStringView), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezHashedString", "ezStringView GetView() const", asMETHOD(ezHashedString, GetView), asCALL_THISCALL));
}
