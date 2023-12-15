#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/ImageFileFormat.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezImageFileFormat);

ezImageFileFormat* ezImageFileFormat::GetReaderFormat(ezStringView sExtension)
{
  for (ezImageFileFormat* pFormat = ezImageFileFormat::GetFirstInstance(); pFormat; pFormat = pFormat->GetNextInstance())
  {
    if (pFormat->CanReadFileType(sExtension))
    {
      return pFormat;
    }
  }

  return nullptr;
}

ezImageFileFormat* ezImageFileFormat::GetWriterFormat(ezStringView sExtension)
{
  for (ezImageFileFormat* pFormat = ezImageFileFormat::GetFirstInstance(); pFormat; pFormat = pFormat->GetNextInstance())
  {
    if (pFormat->CanWriteFileType(sExtension))
    {
      return pFormat;
    }
  }

  return nullptr;
}

ezResult ezImageFileFormat::ReadImageHeader(ezStringView sFileName, ezImageHeader& ref_header)
{
  EZ_LOG_BLOCK("Read Image Header", sFileName);

  EZ_PROFILE_SCOPE(ezPathUtils::GetFileNameAndExtension(sFileName).GetStartPointer());

  ezFileReader reader;
  if (reader.Open(sFileName) == EZ_FAILURE)
  {
    ezLog::Warning("Failed to open image file '{0}'", ezArgSensitive(sFileName, "File"));
    return EZ_FAILURE;
  }

  ezStringView it = ezPathUtils::GetFileExtension(sFileName);

  if (ezImageFileFormat* pFormat = ezImageFileFormat::GetReaderFormat(it.GetStartPointer()))
  {
    if (pFormat->ReadImageHeader(reader, ref_header, it.GetStartPointer()) != EZ_SUCCESS)
    {
      ezLog::Warning("Failed to read image file '{0}'", ezArgSensitive(sFileName, "File"));
      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

  ezLog::Warning("No known image file format for extension '{0}'", it);
  return EZ_FAILURE;
}


