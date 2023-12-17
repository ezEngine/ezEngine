#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/ShaderCompiler/PermutationGenerator.h>

void ezPermutationGenerator::Clear()
{
  m_Permutations.Clear();
}


void ezPermutationGenerator::RemovePermutations(const ezHashedString& sPermVarName)
{
  m_Permutations.Remove(sPermVarName);
}

void ezPermutationGenerator::AddPermutation(const ezHashedString& sName, const ezHashedString& sValue)
{
  EZ_ASSERT_DEV(!sName.IsEmpty(), "");
  EZ_ASSERT_DEV(!sValue.IsEmpty(), "");

  m_Permutations[sName].Insert(sValue);
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

void ezPermutationGenerator::GetPermutation(ezUInt32 uiPerm, ezHybridArray<ezPermutationVar, 16>& out_permVars) const
{
  out_permVars.Clear();

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

    ezPermutationVar& pv = out_permVars.ExpandAndGetRef();
    pv.m_sName = itVariable.Key();
    pv.m_sValue = itValue.Key();
  }
}


