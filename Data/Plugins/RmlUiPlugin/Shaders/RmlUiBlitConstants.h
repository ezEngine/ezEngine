#include <Shaders/Common/GlobalConstants.h>

CONSTANT_BUFFER(ezRmlUiBlitConstants, 4)
{
  FLOAT2(Scale);
  FLOAT2(Offset);
};
