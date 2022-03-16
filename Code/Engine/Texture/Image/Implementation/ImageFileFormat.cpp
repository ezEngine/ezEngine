#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/ImageFileFormat.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezImageFileFormat);

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

ezResult ezImageFileFormat::ReadImageHeader(const char* szFileName, ezImageHeader& header)
{
  EZ_LOG_BLOCK("Read Image Header", szFileName);

  EZ_PROFILE_SCOPE(ezPathUtils::GetFileNameAndExtension(szFileName).GetStartPointer());

  ezFileReader reader;
  if (reader.Open(szFileName) == EZ_FAILURE)
  {
    ezLog::Warning("Failed to open image file '{0}'", ezArgSensitive(szFileName, "File"));
    return EZ_FAILURE;
  }

  ezStringView it = ezPathUtils::GetFileExtension(szFileName);

  if (ezImageFileFormat* pFormat = ezImageFileFormat::GetReaderFormat(it.GetStartPointer()))
  {
    if (pFormat->ReadImageHeader(reader, header, it.GetStartPointer()) != EZ_SUCCESS)
    {
      ezLog::Warning("Failed to read image file '{0}'", ezArgSensitive(szFileName, "File"));
      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

  ezLog::Warning("No known image file format for extension '{0}'", it);
  return EZ_FAILURE;
}

EZ_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageFileFormat);
