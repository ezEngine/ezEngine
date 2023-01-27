#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>

class ezVolumeCollection;

struct EZ_PROCGENPLUGIN_DLL ezProcGenExpressionFunctions
{
  static ezExpressionFunction s_ApplyVolumesFunc;
  static ezExpressionFunction s_GetInstanceSeedFunc;
};

namespace ezProcGenInternal
{
  void ExtractVolumeCollections(const ezWorld& world, const ezBoundingBox& box, const Output& output, ezDeque<ezVolumeCollection>& volumeCollections, ezExpression::GlobalData& globalData);

  void SetInstanceSeed(ezUInt32 uiSeed, ezExpression::GlobalData& globalData);
}
