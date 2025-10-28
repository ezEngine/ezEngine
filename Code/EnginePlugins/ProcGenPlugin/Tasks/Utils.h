#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>

class ezVolumeCollection;

struct EZ_PROCGENPLUGIN_DLL ezProcGenExpressionFunctions
{
  static ezExpressionFunction s_ApplyVolumesFunc;
  static ezExpressionFunction s_GetInstanceSeedFunc;
  static ezExpressionFunction s_SampleCurveFunc;
};

namespace ezProcGenInternal
{
  class GraphSharedData;

  void EZ_PROCGENPLUGIN_DLL ExtractVolumeCollections(const ezWorld& world, const ezBoundingBox& box, const Output& output, ezDeque<ezVolumeCollection>& ref_volumeCollections, ezExpression::GlobalData& ref_globalData);

  void EZ_PROCGENPLUGIN_DLL SetInstanceSeed(ezUInt32 uiSeed, ezExpression::GlobalData& ref_globalData);

  void EZ_PROCGENPLUGIN_DLL SetCurves(const Output& output, ezExpression::GlobalData& ref_globalData);
} // namespace ezProcGenInternal
