#include <Texture/TexturePCH.h>

EZ_STATICLINK_LIBRARY(Texture)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(Texture_Image_Conversions_DXTexConversions);
  EZ_STATICLINK_REFERENCE(Texture_Image_Implementation_ImageEnums);
  EZ_STATICLINK_REFERENCE(Texture_Image_Implementation_ImageFormat);
  EZ_STATICLINK_REFERENCE(Texture_TexConv_Implementation_Processor);
}
