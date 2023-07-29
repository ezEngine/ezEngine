#include <Texture/TexturePCH.h>

#include <Texture/Image/Formats/VdbFileFormat.h>

#include <Texture/Image/Image.h>

namespace
{
  const char ezVdbMagic[] = {' ', 'B', 'D', 'V', 0, 0, 0, 0}; // little endian

  ezResult ReadString(ezStreamReader& inout_stream, ezString& out_string)
  {
    ezUInt32 numBytes = 0;
    EZ_SUCCEED_OR_RETURN(inout_stream.ReadDWordValue(&numBytes));

    ezHybridArray<char, 32> tmp;
    tmp.SetCountUninitialized(numBytes);

    if (inout_stream.ReadBytes(tmp.GetData(), numBytes) != numBytes)
    {
      return EZ_FAILURE;
    }

    out_string = ezStringView(tmp.GetData(), tmp.GetCount());
    return EZ_SUCCESS;
  }

  ezResult SkipString(ezStreamReader& inout_stream)
  {
    ezUInt32 numBytes = 0;
    EZ_SUCCEED_OR_RETURN(inout_stream.ReadDWordValue(&numBytes));

    if (inout_stream.SkipBytes(numBytes) != numBytes)
    {
      return EZ_FAILURE;
    }
    return EZ_SUCCESS;
  }
} // namespace

ezResult ezVdbFileFormat::ReadImageHeader(ezStreamReader& inout_stream, ezImageHeader& ref_header, ezStringView sFileExtension) const
{
  char magic[EZ_ARRAY_SIZE(ezVdbMagic)] = {};
  if (inout_stream.ReadArray(magic).Failed())
  {
    ezLog::Error("Failed to read magic start bytes");
    return EZ_FAILURE;
  }

  if (ezMemoryUtils::Compare(magic, ezVdbMagic, EZ_ARRAY_SIZE(ezVdbMagic)) != 0)
  {
    ezLog::Error("The file is not recognized as a vdb file");
    return EZ_FAILURE;
  }

  ezUInt32 fileVersion = 0;
  if (inout_stream.ReadDWordValue(&fileVersion).Failed())
  {
    ezLog::Error("Failed to read file version");
    return EZ_FAILURE;
  }
  ezLog::Info("VDB file version {}", fileVersion);

  ezUInt32 vdbCreatorVersionMajor = 0, vdbCreatorVersionMinor = 0;
  if (inout_stream.ReadDWordValue(&vdbCreatorVersionMajor).Failed() || inout_stream.ReadDWordValue(&vdbCreatorVersionMinor).Failed())
  {
    ezLog::Error("Failed to read vdb creator version");
    return EZ_FAILURE;
  }

  ezLog::Info("VDB creator version {}.{}", vdbCreatorVersionMajor, vdbCreatorVersionMinor);

  bool hasGridOffsets = false;
  if (inout_stream.ReadBytes(&hasGridOffsets, 1) != 1)
  {
    ezLog::Error("Failed to read has grid offset");
    return EZ_FAILURE;
  }

  char uuid[36] = {};
  if (inout_stream.ReadArray(uuid).Failed())
  {
    ezLog::Error("Failed to read uuid");
    return EZ_FAILURE;
  }

  ezUInt32 numMetadataEntries = 0;
  if (inout_stream.ReadDWordValue(&numMetadataEntries).Failed())
  {
    ezLog::Error("Failed to read metadata num entries");
    return EZ_FAILURE;
  }

  // Skip metadata
  for (ezUInt32 metadataIndex = 0; metadataIndex < numMetadataEntries; metadataIndex++)
  {
    ezString metadataName;
    ezString metadataType;

    if (ReadString(inout_stream, metadataName).Failed() || ReadString(inout_stream, metadataName).Failed())
    {
      ezLog::Error("Failed to read metadata entry name or type");
      return EZ_FAILURE;
    }

    if (metadataType == "string")
    {
      if (SkipString(inout_stream).Failed())
      {
        ezLog::Error("Failed to read value of metadata entry {}", metadataName);
        return EZ_FAILURE;
      }
    }
    else
    {
      ezLog::Error("Metadata entry {} has unknown type '{}'", metadataName, metadataType);
      return EZ_FAILURE;
    }
  }

  ezUInt32 numGrids = 0;
  if(inout_stream.ReadDWordValue(&numGrids).Failed())
  {
    ezLog::Error("Failed to read grid count");
    return EZ_FAILURE;
  }

  return EZ_FAILURE;
}

ezResult ezVdbFileFormat::ReadImage(ezStreamReader& inout_stream, ezImage& ref_image, ezStringView sFileExtension) const
{
  return EZ_FAILURE;
}

ezResult ezVdbFileFormat::WriteImage(ezStreamWriter& inout_stream, const ezImageView& image, ezStringView sFileExtension) const
{
  EZ_ASSERT_NOT_IMPLEMENTED
  return EZ_FAILURE;
}

bool ezVdbFileFormat::CanReadFileType(ezStringView sExtension) const
{
  return sExtension.IsEqual_NoCase("vdb");
}

bool ezVdbFileFormat::CanWriteFileType(ezStringView sExtension) const
{
  return false;
}
