#include <Shaders/Common/GlobalConstants.h>

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
};
