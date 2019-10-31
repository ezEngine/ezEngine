#pragma once

#include <ProcGenPlugin/VM/ExpressionFunctions.h>

class ezVolumeCollection;

struct EZ_PROCGENPLUGIN_DLL ezProcGenExpressionFunctions
{
  static void ApplyVolumes(ezExpression::Inputs inputs, ezExpression::Output output, const ezExpression::GlobalData& globalData);
  static ezResult ApplyVolumesValidate(const ezExpression::GlobalData& globalData);
};

namespace ezProcGenInternal
{
  void ExtractVolumeCollections(const ezWorld& world, const ezBoundingBox& box, const Output& output,
    ezDynamicArray<ezVolumeCollection>& volumeCollections, ezExpression::GlobalData& globalData);
}
