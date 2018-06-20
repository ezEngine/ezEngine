#include <PCH.h>
#include <ProceduralPlacementPlugin/VM/ExpressionFunctions.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Foundation/Containers/HashTable.h>

namespace
{
  struct FunctionInfo
  {
    ezHashedString m_sName;
    ezExpressionFunction m_Func;
  };

  static ezHashTable<ezUInt32, FunctionInfo, ezHashHelper<ezUInt32>, ezStaticAllocatorWrapper> s_ExpressionFunctions;
}

//static
bool ezExpressionFunctionRegistry::RegisterFunction(const char* szName, ezExpressionFunction func)
{
  ezHashedString sName;
  sName.Assign(szName);

  EZ_ASSERT_DEV(!s_ExpressionFunctions.Contains(sName.GetHash()), "A function with the same name (hash) already exists!");

  auto& functionInfo = s_ExpressionFunctions[sName.GetHash()];
  functionInfo.m_sName = sName;
  functionInfo.m_Func = func;

  return true;
}

//static
const char* ezExpressionFunctionRegistry::GetName(ezUInt32 uiNameHash)
{
  FunctionInfo* pFunctionInfo = nullptr;
  if (s_ExpressionFunctions.TryGetValue(uiNameHash, pFunctionInfo))
  {
    return pFunctionInfo->m_sName;
  }

  return nullptr;
}

//static
ezExpressionFunction ezExpressionFunctionRegistry::GetFunction(ezUInt32 uiNameHash)
{
  FunctionInfo* pFunctionInfo = nullptr;
  if (s_ExpressionFunctions.TryGetValue(uiNameHash, pFunctionInfo))
  {
    return pFunctionInfo->m_Func;
  }

  return ezExpressionFunction();
}

//static
ezExpressionFunction ezExpressionFunctionRegistry::GetFunction(const char* szName)
{
  ezUInt32 uiNameHash = ezTempHashedString(szName).GetHash();
  return GetFunction(uiNameHash);
}

//////////////////////////////////////////////////////////////////////////

static void ezExpressionRandom(ezExpression::Inputs inputs, ezExpression::Output output, ezExpression::UserData userData)
{
  auto seeds = inputs[0];

  const ezSimdVec4f* pSeeds = seeds.GetPtr();
  const ezSimdVec4f* pSeedsEnd = pSeeds + seeds.GetCount();
  ezSimdVec4f* pOutput = output.GetPtr();

  while (pSeeds < pSeedsEnd)
  {
    ezSimdVec4i seed = ezSimdVec4i::Truncate(*pSeeds);

    *pOutput = ezSimdRandom::FloatZeroToOne(seed);

    ++pSeeds;
    ++pOutput;
  }
}

EZ_REGISTER_EXPRESSION_FUNCTION("Random", &ezExpressionRandom);
