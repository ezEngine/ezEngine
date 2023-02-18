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
  void ExtractVolumeCollections(const ezWorld& world, const ezBoundingBox& box, const Output& output, ezDeque<ezVolumeCollection>& ref_volumeCollections, ezExpression::GlobalData& ref_globalData);

  void SetInstanceSeed(ezUInt32 uiSeed, ezExpression::GlobalData& ref_globalData);
} // namespace ezProcGenInternal
