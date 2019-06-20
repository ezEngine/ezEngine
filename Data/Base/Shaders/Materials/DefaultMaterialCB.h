#include <Shaders/Common/GlobalConstants.h>

CONSTANT_BUFFER(ezMaterialConstants, 1)
{
  COLOR4F(BaseColor);
  FLOAT1(MetallicValue);
  FLOAT1(ReflectanceValue);
  FLOAT1(RoughnessValue);
  FLOAT1(MaskThreshold);
  BOOL1(UseBaseTexture);
  BOOL1(UseMetallicTexture);
  BOOL1(UseNormalAndRoughnessTexture);
  BOOL1(UseEmissiveTexture);
  COLOR4F(EmissiveColor);
  BOOL1(UseOcclusionTexture);
};
