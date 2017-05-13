#pragma once

#include <ModelImporter/Importers/PbrtImporter_Declarations.h>

namespace ezModelImporter
{
  using namespace Pbrt;

  namespace PbrtParseHelper
  {
    void SkipCurrentLine(ezStringView& string);
    void SkipWhiteSpaces(ezStringView& string);

    ezStringView ReadCommandName(ezStringView& remainingSceneText);

    ezStringView ReadBlock(ezStringView& remainingSceneText, char blockStart, char blockEnd);

    ParamType GetParamType(const ezStringView& type);

    ezResult ParseVec3(ezStringView& params, ezVec3& out);
    ezResult ParseFloats(ezStringView& params, ezArrayPtr<float> outFloats, int numExpectedFloats);

    Parameter::DataArray ParseParameterBlock(ParamType type, ezStringView& remainingSceneText);
  }
}
