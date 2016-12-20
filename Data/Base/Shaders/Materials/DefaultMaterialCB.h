#include <Shaders/Common/GlobalConstants.h>

#ifndef VSE_CONSTANTS
#define VSE_CONSTANTS
#endif

CONSTANT_BUFFER(ezMaterialConstants, 1)
{
  COLOR4F(BaseColor);
  FLOAT1(MetallicValue);
  FLOAT1(ReflectanceValue);
  FLOAT1(RoughnessValue);
  FLOAT1(MaskThreshold);
  BOOL(UseBaseTexture);
  BOOL(UseMetallicTexture);
  BOOL(UseNormalAndRoughnessTexture);
  BOOL(UseEmissiveTexture);
  COLOR4F(EmissiveColor);
  BOOL(UseOcclusionTexture);

  // Insert custom Visual Shader parameters here
  VSE_CONSTANTS
};
