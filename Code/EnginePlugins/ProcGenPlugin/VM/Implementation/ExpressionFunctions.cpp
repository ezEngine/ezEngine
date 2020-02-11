#include <ProcGenPluginPCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/SimdMath/SimdNoise.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <ProcGenPlugin/VM/ExpressionFunctions.h>

//static
void ezDefaultExpressionFunctions::Random(ezExpression::Inputs inputs, ezExpression::Output output, const ezExpression::GlobalData& globalData)
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

namespace
{
  static ezSimdPerlinNoise s_PerlinNoise(12345);
}

//static
void ezDefaultExpressionFunctions::PerlinNoise(ezExpression::Inputs inputs, ezExpression::Output output, const ezExpression::GlobalData& globalData)
{
  const ezSimdVec4f* pPosX = inputs[0].GetPtr();
  const ezSimdVec4f* pPosY = inputs[1].GetPtr();
  const ezSimdVec4f* pPosZ = inputs[2].GetPtr();
  const ezSimdVec4f* pPosXEnd = pPosX + inputs[0].GetCount();

  ezUInt32 uiNumOcataves = ezSimdVec4i::Truncate(inputs[3][0]).x();

  ezSimdVec4f* pOutput = output.GetPtr();

  while (pPosX < pPosXEnd)
  {
    *pOutput = s_PerlinNoise.NoiseZeroToOne(*pPosX, *pPosY, *pPosZ, uiNumOcataves);

    ++pPosX;
    ++pPosY;
    ++pPosZ;
    ++pOutput;
  }
}
