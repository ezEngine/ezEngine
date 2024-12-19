#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/ImageFileFormat.h>

const ezImageFileFormat* ezImageFileFormat::GetReaderFormat(ezStringView sExtension)
{
  for (auto format = ezRegisteredImageFileFormat::GetFirstInstance(); format != nullptr; format = format->GetNextInstance())
  {
    if (format->GetFormatType().CanReadFileType(sExtension))
    {
      return &format->GetFormatType();
    }
  }

  return nullptr;
}

const ezImageFileFormat* ezImageFileFormat::GetWriterFormat(ezStringView sExtension)
{
  for (auto format = ezRegisteredImageFileFormat::GetFirstInstance(); format != nullptr; format = format->GetNextInstance())
  {
    if (format->GetFormatType().CanWriteFileType(sExtension))
    {
      return &format->GetFormatType();
    }
  }

  return nullptr;
}

ezResult ezImageFileFormat::ReadImageHeader(ezStringView sFileName, ezImageHeader& ref_header)
{
  EZ_LOG_BLOCK("Read Image Header", sFileName);

  EZ_PROFILE_SCOPE(ezPathUtils::GetFileNameAndExtension(sFileName));

  ezFileReader reader;
  if (reader.Open(sFileName) == EZ_FAILURE)
  {
    ezLog::Warning("Failed to open image file '{0}'", ezArgSensitive(sFileName, "File"));
    return EZ_FAILURE;
  }

  ezStringView it = ezPathUtils::GetFileExtension(sFileName);

  if (const ezImageFileFormat* pFormat = ezImageFileFormat::GetReaderFormat(it))
  {
    if (pFormat->ReadImageHeader(reader, ref_header, it) != EZ_SUCCESS)
    {
      ezLog::Warning("Failed to read image file '{0}'", ezArgSensitive(sFileName, "File"));
      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

  ezLog::Warning("No known image file format for extension '{0}'", it);
  return EZ_FAILURE;
}

//////////////////////////////////////////////////////////////////////////

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezRegisteredImageFileFormat);

ezRegisteredImageFileFormat::ezRegisteredImageFileFormat() = default;
ezRegisteredImageFileFormat::~ezRegisteredImageFileFormat() = default;
