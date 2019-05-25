#include <Shaders/Common/GlobalConstants.h>

CONSTANT_BUFFER(ezMaterialConstants, 1)
{
  COLOR4F(BaseColor);
  FLOAT1(MetallicValue);
  FLOAT1(ReflectanceValue);
  FLOAT1(RoughnessValue);
  FLOAT1(MaskThreshold);
  BOOLEAN(UseBaseTexture);
  BOOLEAN(UseMetallicTexture);
  BOOLEAN(UseNormalAndRoughnessTexture);
  BOOLEAN(UseEmissiveTexture);
  COLOR4F(EmissiveColor);
  BOOLEAN(UseOcclusionTexture);
};
