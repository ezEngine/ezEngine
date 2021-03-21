#include <ProcGenPluginPCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/SimdMath/SimdNoise.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <ProcGenPlugin/VM/ExpressionFunctions.h>

// static
void ezDefaultExpressionFunctions::Random(ezExpression::Inputs inputs, ezExpression::Output output, const ezExpression::GlobalData& globalData)
{
  ezArrayPtr<const ezSimdVec4f> positions = inputs[0];
  const ezSimdVec4f* pPositions = positions.GetPtr();
  const ezSimdVec4f* pPositionsEnd = pPositions + positions.GetCount();
  ezSimdVec4f* pOutput = output.GetPtr();

  if (inputs.GetCount() >= 2)
  {
    ezArrayPtr<const ezSimdVec4f> seeds = inputs[1];
    const ezSimdVec4f* pSeeds = seeds.GetPtr();

    while (pPositions < pPositionsEnd)
    {
      ezSimdVec4i pos = ezSimdVec4i::Truncate(*pPositions);
      ezSimdVec4u seed = ezSimdVec4u::Truncate(*pSeeds);

      *pOutput = ezSimdRandom::FloatZeroToOne(pos, seed);

      ++pPositions;
      ++pSeeds;
      ++pOutput;
    }
  }
  else
  {
    while (pPositions < pPositionsEnd)
    {
      ezSimdVec4i pos = ezSimdVec4i::Truncate(*pPositions);

      *pOutput = ezSimdRandom::FloatZeroToOne(pos);

      ++pPositions;
      ++pOutput;
    }
  }
}

namespace
{
  static ezSimdPerlinNoise s_PerlinNoise(12345);
}

// static
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
