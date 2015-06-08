#include <Shaders/Common/GlobalConstants.h>

CONSTANT_BUFFER(MaterialCB, 1)
{
  COLOR(MeshColor);
  INT1(PickingHighlightID);
};