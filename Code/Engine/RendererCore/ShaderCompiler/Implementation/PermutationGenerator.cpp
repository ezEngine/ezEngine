#include <RendererCore/PCH.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <Foundation/Algorithm/Hashing.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <CoreUtils/Scripting/LuaWrapper.h>
#include <CoreUtils/CodeUtils/Preprocessor.h>

void ezPermutationGenerator::AddPermutation(const char* szVariable, const char* szValue)
{
  ezStringBuilder sVar, sVal;
  sVar = szVariable;
  sVar.ToUpper();
  sVal = szValue;
  sVal.ToUpper();

  m_Permutations[sVar].Insert(sVal);
}

ezUInt32 ezPermutationGenerator::GetPermutationCount() const
{
  ezUInt32 uiPermutations = 1;

  for (auto it = m_Permutations.GetIterator(); it.IsValid(); ++it)
  {
    uiPermutations *= it.Value().GetCount();
  }

  return uiPermutations;
}

void ezPermutationGenerator::GetPermutation(ezUInt32 uiPerm, ezHybridArray<PermutationVar, 16>& out_PermVars) const
{
  out_PermVars.Clear();

  for (auto itVariable = m_Permutations.GetIterator(); itVariable.IsValid(); ++itVariable)
  {
    const ezUInt32 uiValues = itVariable.Value().GetCount();
    ezUInt32 uiUseValue = uiPerm % uiValues;

    uiPerm /= uiValues;

    auto itValue = itVariable.Value().GetIterator();

    for (; uiUseValue > 0; --uiUseValue)
    {
      ++itValue;
    }

    PermutationVar pv;
    pv.m_sVariable = itVariable.Key();
    pv.m_sValue = itValue.Key();

    out_PermVars.PushBack(pv);
  }
}

ezUInt32 ezPermutationGenerator::GetHash(const ezHybridArray<PermutationVar, 16>& PermVars)
{
  ezStringBuilder s;

  for (ezUInt32 i = 0; i < PermVars.GetCount(); ++i)
    s.AppendFormat("%s = %s;", PermVars[i].m_sVariable.GetData(), PermVars[i].m_sValue.GetData());

  return ezHashing::MurmurHash(ezHashing::StringWrapper(s.GetData()));
}

static bool PermutationUsed(const ezString& sPermutations, const char* szPermutation)
{
  return sPermutations.FindWholeWord_NoCase(szPermutation, ezStringUtils::IsIdentifierDelimiter_C_Code) != nullptr;
}

void ezPermutationGenerator::RemoveUnusedPermutations(const ezString& sUsedPermutations)
{
  for (auto it = m_Permutations.GetIterator(); it.IsValid(); )
  {
    if (!PermutationUsed(sUsedPermutations, it.Key().GetData()))
      it = m_Permutations.Remove(it);
    else
      ++it;
  }
}

int ezPermutationGenerator::LUAFUNC_add(lua_State* state)
{
  ezLuaWrapper s(state);

  ezPermutationGenerator* pGenerator = (ezPermutationGenerator*) s.GetFunctionLightUserData();

  const ezString sVar   = s.GetStringParameter(0);
  const ezString sValue = s.GetStringParameter(1);

  pGenerator->AddPermutation(sVar.GetData(), sValue.GetData());

  return s.ReturnToScript();
}

ezResult ezPermutationGenerator::ReadFromFile(const char* szFile, const char* szPlatform)
{
  ezStringBuilder sTemp = szPlatform;
  sTemp.Append(" 1");

  ezPreprocessor pp;
  pp.SetLogInterface(ezGlobalLog::GetInstance());
  pp.SetPassThroughLine(false);
  pp.SetPassThroughPragma(false);
  pp.AddCustomDefine(sTemp.GetData());

  if (pp.Process(szFile, sTemp, false).Failed())
    return EZ_FAILURE;

  ezLuaWrapper script;
  script.RegisterCFunction("add", LUAFUNC_add, this);
  script.ExecuteString(sTemp.GetData(), "chunk", ezGlobalLog::GetInstance());

  return EZ_SUCCESS;
}

bool ezPermutationGenerator::IsValueAllowed(const char* szVariable, const char* szValue) const
{
  ezMap<ezString, ezSet<ezString> >::ConstIterator it = m_Permutations.Find(szVariable);

  if (!it.IsValid())
    return false;

  return it.Value().Find(szValue).IsValid();
}





EZ_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_PermutationGenerator);

