#include <Shaders/Common/GlobalConstants.h>

CONSTANT_BUFFER(MaterialCB, 0)
{
  MAT4(ModelMatrix);
  MAT4(ViewProjectionMatrix);
  COLOR(MeshColor);
  INT1(PickingHighlightID);
};