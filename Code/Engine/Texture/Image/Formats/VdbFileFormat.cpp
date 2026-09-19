#include <Texture/TexturePCH.h>

#include <Texture/Image/Formats/VdbFileFormat.h>

#include <Texture/Image/Image.h>

EZ_STATICLINK_FORCE static ezImageFileFormatRegistrator<ezVdbFileFormat> g_vdbFormat;

namespace
{
  const ezUInt8 ezVdbMagic[] = {' ', 'B', 'D', 'V', 0, 0, 0, 0}; // little endian

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

  struct ezVdbCompression
  {
    using StorageType = ezUInt32;

    enum Enum
    {
      Zip = EZ_BIT(0),
      ActiveMask = EZ_BIT(2),
      Blosc = EZ_BIT(3),
      Default = 0
    };

    struct Bits
    {
      StorageType Zip : 1;
      StorageType ActiveMask : 1;
      StorageType Blosc : 1;
    };
  };

  EZ_DECLARE_FLAGS_OPERATORS(ezVdbCompression);
} // namespace

ezResult ezVdbFileFormat::ReadImageHeader(ezStreamReader& inout_stream, ezImageHeader& ref_header, ezStringView sFileExtension) const
{
  EZ_IGNORE_UNUSED(ref_header);
  EZ_IGNORE_UNUSED(sFileExtension);

  // VDB contains absolute offsets within the file, so read everything into memory first, then use a memory reader to parse the file
  ezDefaultMemoryStreamStorage storage;
  {
    ezUInt8 buffer[4096];
    ezMemoryStreamWriter writer(&storage);
    while(ezUInt64 numBytes = inout_stream.ReadBytes(buffer, EZ_ARRAY_SIZE(buffer)))
    {
      if(writer.WriteBytes(buffer, numBytes).Failed())
      {
        ezLog::Error("Creating in memory copy of VDB file failed");
        return EZ_FAILURE;
      }
    }
  }


  // Parse
  ezMemoryStreamReader reader(&storage);

  ezUInt8 magic[EZ_ARRAY_SIZE(ezVdbMagic)] = {};
  if (reader.ReadBytes(magic, sizeof(magic)) != sizeof(magic))
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
  if (reader.ReadDWordValue(&fileVersion).Failed())
  {
    ezLog::Error("Failed to read file version");
    return EZ_FAILURE;
  }
  ezLog::Info("VDB file version {}", fileVersion);

  ezUInt32 vdbCreatorVersionMajor = 0, vdbCreatorVersionMinor = 0;
  if (reader.ReadDWordValue(&vdbCreatorVersionMajor).Failed() || reader.ReadDWordValue(&vdbCreatorVersionMinor).Failed())
  {
    ezLog::Error("Failed to read vdb creator version");
    return EZ_FAILURE;
  }

  ezLog::Info("VDB creator version {}.{}", vdbCreatorVersionMajor, vdbCreatorVersionMinor);

  bool hasGridOffsets = false;
  if (reader.ReadBytes(&hasGridOffsets, 1) != 1)
  {
    ezLog::Error("Failed to read has grid offset");
    return EZ_FAILURE;
  }

  ezUInt8 uuid[36] = {};
  if (reader.ReadBytes(uuid, sizeof(uuid)) != sizeof(uuid))
  {
    ezLog::Error("Failed to read uuid");
    return EZ_FAILURE;
  }

  ezUInt32 numMetadataEntries = 0;
  if (reader.ReadDWordValue(&numMetadataEntries).Failed())
  {
    ezLog::Error("Failed to read metadata num entries");
    return EZ_FAILURE;
  }

  // Skip metadata
  for (ezUInt32 metadataIndex = 0; metadataIndex < numMetadataEntries; metadataIndex++)
  {
    ezString metadataName;
    ezString metadataType;

    if (ReadString(reader, metadataName).Failed() || ReadString(reader, metadataType).Failed())
    {
      ezLog::Error("Failed to read metadata entry name or type");
      return EZ_FAILURE;
    }

    if (metadataType == "string")
    {
      if (SkipString(reader).Failed())
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
  if(reader.ReadDWordValue(&numGrids).Failed())
  {
    ezLog::Error("Failed to read grid count");
    return EZ_FAILURE;
  }

  if(numGrids != 1)
  {
    ezLog::Error("VDB reader currently only supports a single grid, file has {} grids.", numGrids);
    return EZ_FAILURE;
  }

  ezString gridName;
  if(reader.ReadString(gridName).Failed())
  {
    ezLog::Error("Failed to read name of grid 0");
    return EZ_FAILURE;
  }

  ezString gridType;
  if(reader.ReadString(gridType).Failed())
  {
    ezLog::Error("Failed to read grid 0 type");
    return EZ_FAILURE;
  }

  if(gridType != "Tree_float_5_4_3"_ezsv)
  {
    ezLog::Error("Grid type '{}' is not supported", gridType);
    return EZ_FAILURE;
  }

  ezUInt32 instanceParent;

  if(reader.ReadDWordValue(&instanceParent).Failed() || instanceParent != 0)
  {
    ezLog::Error("Failed to read instanceParent or unsupported value");
    return EZ_FAILURE;
  }

  ezUInt64 currentOffset = reader.GetReadPosition();

  ezUInt64 gridDescriptorOffset;
  if(reader.ReadQWordValue(&gridDescriptorOffset).Failed() || gridDescriptorOffset < currentOffset)
  {
    ezLog::Error("Failed to read gridDescriptorOffset");
    return EZ_FAILURE;
  }
  gridDescriptorOffset -= currentOffset;
  if(gridDescriptorOffset < sizeof(ezUInt64) * 3)
  {
    ezLog::Error("Unexpected grid descriptor offset value");
    return EZ_FAILURE;
  }

  ezUInt64 startOfGridDataOffset, endOfGridDataOffset;
  if(reader.ReadQWordValue(&startOfGridDataOffset).Failed() || reader.ReadQWordValue(&endOfGridDataOffset).Failed())
  {
    ezLog::Error("Failed to read grid data offsets");
    return EZ_FAILURE;
  }

  // we just read 3 ezUInt64s
  gridDescriptorOffset -= sizeof(ezUInt64) * 3;

  // We want to read the grid descriptor next, skip any data we don't need
  if(gridDescriptorOffset > 0)
  {
    if(reader.SkipBytes(gridDescriptorOffset) != gridDescriptorOffset)
    {
      return EZ_FAILURE;
    }
  }

  ezUInt32 compressionValue;
  if(reader.ReadDWordValue(&compressionValue).Failed())
  {
    ezLog::Error("Failed to read isUsingCompression integer or unsupported compression type");
    return EZ_FAILURE;
  }
  ezBitflags<ezVdbCompression> compression;
  compression.SetValue(compressionValue);

  ezUInt32 numGridMetadata;
  if(reader.ReadDWordValue(&numGridMetadata).Failed())
  {
    ezLog::Error("Failed to read numGridMetadata");
    return EZ_FAILURE;
  }

  for(ezUInt32 metadataIndex = 0; metadataIndex < numGridMetadata; ++metadataIndex)
  {
    ezString metadataName;
    ezString metadataType;

    if(reader.ReadString(metadataName).Failed() || reader.ReadString(metadataName).Failed())
    {
      ezLog::Error("Failed to read grid metadata entry");
      return EZ_FAILURE;
    }

    if (metadataType == "string")
    {
      if (SkipString(reader).Failed())
      {
        ezLog::Error("Failed to read value of grid metadata entry {}", metadataName);
        return EZ_FAILURE;
      }
    }
    else
    {
      ezLog::Error("Grid metadata entry {} has unknown type '{}'", metadataName, metadataType);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezVdbFileFormat::ReadImage(ezStreamReader& inout_stream, ezImage& ref_image, ezStringView sFileExtension) const
{
  EZ_IGNORE_UNUSED(sFileExtension);
  EZ_IGNORE_UNUSED(ref_image);
  EZ_IGNORE_UNUSED(inout_stream);

  ezImageHeader header;
  if(ReadImageHeader(inout_stream, header, sFileExtension).Failed())
    return EZ_FAILURE;

  ezImage image;
  image.ResetAndAlloc(header);


  ref_image.ResetAndMove(std::move(image));

  return EZ_FAILURE;
}

ezResult ezVdbFileFormat::WriteImage(ezStreamWriter& inout_stream, const ezImageView& image, ezStringView sFileExtension) const
{
  EZ_IGNORE_UNUSED(sFileExtension);
  EZ_IGNORE_UNUSED(image);
  EZ_IGNORE_UNUSED(inout_stream);
  EZ_ASSERT_NOT_IMPLEMENTED
  return EZ_FAILURE;
}

bool ezVdbFileFormat::CanReadFileType(ezStringView sExtension) const
{
  return sExtension.IsEqual_NoCase("vdb");
}

bool ezVdbFileFormat::CanWriteFileType(ezStringView sExtension) const
{
  EZ_IGNORE_UNUSED(sExtension);
  return false;
}

EZ_STATICLINK_FILE(Texture, Texture_Image_Formats_VdbFileFormat);
