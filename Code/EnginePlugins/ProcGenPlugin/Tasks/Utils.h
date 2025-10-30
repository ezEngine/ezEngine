#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>

class ezVolumeCollection;

struct EZ_PROCGENPLUGIN_DLL ezProcGenExpressionFunctions
{
  static ezExpressionFunction s_ApplyVolumesFunc;
  static ezExpressionFunction s_GetInstanceSeedFunc;
  static ezExpressionFunction s_SampleCurveFunc;
};

class EZ_PROCGENPLUGIN_DLL ezProcGenGlobalData
{
public:
  static void ExtractVolumeCollections(const ezWorld& world, const ezBoundingBox& box, const ezProcGenInternal::Output& output, ezDeque<ezVolumeCollection>& ref_volumeCollections, ezExpression::GlobalData& ref_globalData);

  static void SetInstanceSeed(ezUInt32 uiSeed, ezExpression::GlobalData& ref_globalData);

  static void SetCurves(const ezProcGenInternal::Output& output, ezExpression::GlobalData& ref_globalData);
};
