#include <PCH.h>
#include <ProceduralPlacementPlugin/VM/ExpressionFunctions.h>
#include <Foundation/SimdMath/SimdVec4i.h>

namespace
{
  struct FunctionInfo
  {
    ezHashedString m_sName;
    ezExpressionFunction m_Func;
  };

  static ezHashTable<ezUInt32, FunctionInfo> s_ExpressionFunctions;
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

    // Rand Xor Shift
    seed = seed ^ (seed << 13);
    seed = seed ^ (seed >> 17);
    seed = seed ^ (seed << 5);

    // Wang Hash
    seed = (seed ^ ezSimdVec4i(61)) ^ (seed >> 16);
    seed = seed.CompMul(ezSimdVec4i(9));
    seed = seed ^ (seed >> 4);
    seed = seed.CompMul(ezSimdVec4i(0x27d4eb2d));
    seed = seed ^ (seed >> 15);

    // Convert to 0..1 range
    ezSimdVec4f value = seed.ToFloat() * (1.0f / 2147483648.0f);

    *pOutput = value;

    ++pSeeds;
    ++pOutput;
  }
}

EZ_REGISTER_EXPRESSION_FUNCTION("Random", &ezExpressionRandom);
