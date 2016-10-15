#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/Variant.h>

namespace ezModelImporter
{
  namespace Pbrt
  {
    enum class ParamType
    {
      FLOAT,
      STRING,
      TEXTURE,
      SPECTRUM,
      INT,
      VECTOR3,
      COLOR,

      INVALID
    };

    struct Parameter
    {
      typedef ezHybridArray<ezVariant, 1> DataArray;

      ParamType type;
      ezStringView name;
      DataArray data;
    };

    class ParseContext;
  }
}
