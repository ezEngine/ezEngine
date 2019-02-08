#include <TexturePCH.h>

#include <Texture/Image/Formats/ImageFileFormat.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezImageFileFormat);

EZ_STATICLINK_FILE(Foundation, Foundation_Image_Implementation_ImageFileFormat);

ezImageFileFormat* ezImageFileFormat::GetReaderFormat(const char* extension)
{
  for (ezImageFileFormat* pFormat = ezImageFileFormat::GetFirstInstance(); pFormat; pFormat = pFormat->GetNextInstance())
  {
    if (pFormat->CanReadFileType(extension))
    {
      return pFormat;
    }
  }

  return nullptr;
}

ezImageFileFormat* ezImageFileFormat::GetWriterFormat(const char* extension)
{
  for (ezImageFileFormat* pFormat = ezImageFileFormat::GetFirstInstance(); pFormat; pFormat = pFormat->GetNextInstance())
  {
    if (pFormat->CanWriteFileType(extension))
    {
      return pFormat;
    }
  }

  return nullptr;
}

EZ_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageFileFormat);

