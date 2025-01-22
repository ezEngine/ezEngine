#include <Shaders/Common/GlobalConstants.h>

CONSTANT_BUFFER(ezRmlUiConstants, 4)
{
  MAT4(UiTransform);
  FLOAT4(UiTranslation);
  BOOL1(TextureNeedsAlphaMultiplication);
};
