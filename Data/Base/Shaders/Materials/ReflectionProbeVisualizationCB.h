#include <Shaders/Common/GlobalConstants.h>

CONSTANT_BUFFER(ezMaterialConstants, 1)
{
  INT1(MipLevel);
  INT1(ReflectionProbeIndex);
};

