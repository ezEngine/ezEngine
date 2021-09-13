#pragma once

#include <ProcGenPlugin/VM/ExpressionFunctions.h>

class ezVolumeCollection;
class ezImageCollection;

struct EZ_PROCGENPLUGIN_DLL ezProcGenExpressionFunctions
{
  static void ApplyVolumes(ezExpression::Inputs inputs, ezExpression::Output output, const ezExpression::GlobalData& globalData);
  static ezResult ApplyVolumesValidate(const ezExpression::GlobalData& globalData);

  static void SampleImages(ezExpression::Inputs inputs, ezExpression::Output output, const ezExpression::GlobalData& globalData);
  static ezResult SampleImagesValidate(const ezExpression::GlobalData& globalData);
};

namespace ezProcGenInternal
{
  void ExtractVolumeCollections(const ezWorld& world, const ezBoundingBox& box, const Output& output, ezDeque<ezVolumeCollection>& volumeCollections, ezExpression::GlobalData& globalData);
  void ExtractImageCollections(const ezWorld& world, const ezBoundingBox& box, const Output& output, ezDeque<ezImageCollection>& imageCollections, ezExpression::GlobalData& globalData);
}
