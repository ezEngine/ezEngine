#include <Texture/TexturePCH.h>

EZ_STATICLINK_LIBRARY(Texture)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(Texture_Image_Conversions_BC7EncConversions);
  EZ_STATICLINK_REFERENCE(Texture_Image_Conversions_DXTConversions);
  EZ_STATICLINK_REFERENCE(Texture_Image_Conversions_DXTexConversions);
  EZ_STATICLINK_REFERENCE(Texture_Image_Conversions_DXTexCpuConversions);
  EZ_STATICLINK_REFERENCE(Texture_Image_Conversions_PixelConversions);
  EZ_STATICLINK_REFERENCE(Texture_Image_Conversions_PlanarConversions);
  EZ_STATICLINK_REFERENCE(Texture_Image_Formats_BmpFileFormat);
  EZ_STATICLINK_REFERENCE(Texture_Image_Formats_DdsFileFormat);
  EZ_STATICLINK_REFERENCE(Texture_Image_Formats_ExrFileFormat);
  EZ_STATICLINK_REFERENCE(Texture_Image_Formats_StbImageFileFormats);
  EZ_STATICLINK_REFERENCE(Texture_Image_Formats_TgaFileFormat);
  EZ_STATICLINK_REFERENCE(Texture_Image_Formats_WicFileFormat);
  EZ_STATICLINK_REFERENCE(Texture_Image_Implementation_ImageEnums);
  EZ_STATICLINK_REFERENCE(Texture_Image_Implementation_ImageFormat);
  EZ_STATICLINK_REFERENCE(Texture_TexConv_Implementation_Processor);
}
