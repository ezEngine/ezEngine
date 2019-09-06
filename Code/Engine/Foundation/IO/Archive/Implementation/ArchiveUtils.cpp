#include <FoundationPCH.h>

#include <Foundation/IO/Archive/ArchiveUtils.h>

#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/IO/MemoryStream.h>

ezResult ezArchiveUtils::WriteHeader(ezStreamWriter& stream)
{
  const char* szTag = "EZARCHIVE";
  EZ_SUCCEED_OR_RETURN(stream.WriteBytes(szTag, 10));

  const ezUInt8 uiArchiveVersion = 1;
  stream << uiArchiveVersion;

  const ezUInt8 uiPadding[5] = {0, 0, 0, 0, 0};
  EZ_SUCCEED_OR_RETURN(stream.WriteBytes(uiPadding, 5));

  return EZ_SUCCESS;
}

ezResult ezArchiveUtils::ReadHeader(ezStreamReader& stream, ezUInt8& out_uiVersion)
{
  char szTag[10];
  stream.ReadBytes(szTag, 10);

  if (!ezStringUtils::IsEqual(szTag, "EZARCHIVE"))
    return EZ_FAILURE;

  stream >> out_uiVersion;

  if (out_uiVersion != 1)
    return EZ_FAILURE;

  ezUInt8 uiPadding[5];
  stream.ReadBytes(uiPadding, 5);

  const ezUInt8 uiZeroPadding[5] = {0, 0, 0, 0, 0};

  if (ezMemoryUtils::ByteCompare<ezUInt8>(uiPadding, uiZeroPadding, 5) != 0)
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult ezArchiveUtils::WriteEntry(ezStreamWriter& stream, const char* szAbsSourcePath, ezUInt32 uiPathStringOffset,
  ezArchiveCompressionMode compression, ezArchiveEntry& tocEntry, ezUInt64& inout_uiCurrentStreamPosition,
  FileWriteProgressCallback progress /*= FileWriteProgressCallback()*/)
{
  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(szAbsSourcePath, 1024 * 1024));

  const ezUInt64 uiMaxBytes = file.GetFileSize();

  ezUInt8 uiTemp[1024 * 64];

  tocEntry.m_uiPathStringOffset = uiPathStringOffset;
  tocEntry.m_uiDataStartOffset = inout_uiCurrentStreamPosition;
  tocEntry.m_uiUncompressedDataSize = 0;

  ezStreamWriter* pWriter = &stream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  ezCompressedStreamWriterZstd zstdWriter;
#endif

  switch (compression)
  {
    case ezArchiveCompressionMode::Uncompressed:
      break;

    case ezArchiveCompressionMode::Compressed_zstd:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      zstdWriter.SetOutputStream(&stream);
      pWriter = &zstdWriter;
#else
      compression = ezArchiveCompressionMode::Uncompressed;
#endif
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  tocEntry.m_CompressionMode = compression;

  ezUInt64 uiRead = 0;
  while (true)
  {
    uiRead = file.ReadBytes(uiTemp, EZ_ARRAY_SIZE(uiTemp));

    if (uiRead == 0)
      break;

    tocEntry.m_uiUncompressedDataSize += uiRead;

    if (progress.IsValid())
    {
      if (!progress(tocEntry.m_uiUncompressedDataSize, uiMaxBytes))
        return EZ_FAILURE;
    }

    EZ_SUCCEED_OR_RETURN(pWriter->WriteBytes(uiTemp, uiRead));
  }


  switch (compression)
  {
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    case ezArchiveCompressionMode::Compressed_zstd:
      EZ_SUCCEED_OR_RETURN(zstdWriter.FinishCompressedStream());
      tocEntry.m_uiStoredDataSize = zstdWriter.GetWrittenBytes();
      break;
#endif

    case ezArchiveCompressionMode::Uncompressed:
    default:
      tocEntry.m_uiStoredDataSize = tocEntry.m_uiUncompressedDataSize;
      break;
  }

  inout_uiCurrentStreamPosition += tocEntry.m_uiStoredDataSize;

  return EZ_SUCCESS;
}

ezResult ezArchiveUtils::WriteEntryOptimal(ezStreamWriter& stream, const char* szAbsSourcePath, ezUInt32 uiPathStringOffset,
  ezArchiveCompressionMode compression, ezArchiveEntry& tocEntry, ezUInt64& inout_uiCurrentStreamPosition,
  FileWriteProgressCallback progress /*= FileWriteProgressCallback()*/)
{
  if (compression == ezArchiveCompressionMode::Uncompressed)
  {
    return WriteEntry(stream, szAbsSourcePath, uiPathStringOffset, ezArchiveCompressionMode::Uncompressed, tocEntry, inout_uiCurrentStreamPosition, progress);
  }
  else
  {
    ezMemoryStreamStorage storage;
    ezMemoryStreamWriter writer(&storage);

    ezUInt64 streamPos = inout_uiCurrentStreamPosition;
    EZ_SUCCEED_OR_RETURN(WriteEntry(writer, szAbsSourcePath, uiPathStringOffset, compression, tocEntry, streamPos, progress));

    if (tocEntry.m_uiStoredDataSize * 12 >= tocEntry.m_uiUncompressedDataSize * 10)
    {
      // less than 20% size saving -> go uncompressed
      return WriteEntry(stream, szAbsSourcePath, uiPathStringOffset, ezArchiveCompressionMode::Uncompressed, tocEntry, inout_uiCurrentStreamPosition, progress);
    }
    else
    {
      stream.WriteBytes(storage.GetData(), storage.GetStorageSize());
      inout_uiCurrentStreamPosition = streamPos;

      return EZ_SUCCESS;
    }
  }
}

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT

class ezCompressedStreamReaderWithSource : public ezCompressedStreamReaderZstd
{
public:
  ezRawMemoryStreamReader m_Source;
};

#endif

ezUniquePtr<ezStreamReader> ezArchiveUtils::CreateEntryReader(const ezArchiveEntry& entry, const void* pStartOfArchiveData)
{
  ezUniquePtr<ezStreamReader> reader;

  switch (entry.m_CompressionMode)
  {
    case ezArchiveCompressionMode::Uncompressed:
    {
      reader = EZ_DEFAULT_NEW(ezRawMemoryStreamReader);
      ezRawMemoryStreamReader* pRawReader = static_cast<ezRawMemoryStreamReader*>(reader.Borrow());
      ConfigureRawMemoryStreamReader(entry, pStartOfArchiveData, *pRawReader);
      break;
    }

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    case ezArchiveCompressionMode::Compressed_zstd:
    {
      reader = EZ_DEFAULT_NEW(ezCompressedStreamReaderWithSource);
      ezCompressedStreamReaderWithSource* pRawReader = static_cast<ezCompressedStreamReaderWithSource*>(reader.Borrow());
      ConfigureRawMemoryStreamReader(entry, pStartOfArchiveData, pRawReader->m_Source);
      pRawReader->SetInputStream(&pRawReader->m_Source);
      break;
    }
#endif

    default:
      EZ_REPORT_FAILURE("Archive entry compression mode '{}' is not supported by ezArchiveReader", (int)entry.m_CompressionMode);
      break;
  }

  return std::move(reader);
}

void ezArchiveUtils::ConfigureRawMemoryStreamReader(
  const ezArchiveEntry& entry, const void* pStartOfArchiveData, ezRawMemoryStreamReader& memReader)
{
  memReader.Reset(ezMemoryUtils::AddByteOffset(pStartOfArchiveData, entry.m_uiDataStartOffset), entry.m_uiStoredDataSize);
}

ezResult ezArchiveUtils::AppendTOC(ezStreamWriter& stream, const ezArchiveTOC& toc)
{
  ezMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);

  toc.Serialize(writer);

  EZ_SUCCEED_OR_RETURN(stream.WriteBytes(storage.GetData(), storage.GetStorageSize()));

  // append the size of the TOC data
  const ezUInt32 uiTocSize = storage.GetStorageSize();
  stream << uiTocSize;

  return EZ_SUCCESS;
}

ezResult ezArchiveUtils::ExtractTOC(ezMemoryMappedFile& memFile, ezArchiveTOC& toc)
{
  const ezUInt32 uiTocSize = *static_cast<const ezUInt32*>(memFile.GetReadPointer(sizeof(ezUInt32), ezMemoryMappedFile::OffsetBase::End));

  const ezUInt32 uiTocStart = uiTocSize + sizeof(ezUInt32);

  const void* pTocStart = memFile.GetReadPointer(uiTocStart, ezMemoryMappedFile::OffsetBase::End);

  ezRawMemoryStreamReader tocReader(pTocStart, uiTocSize);

  if (toc.Deserialize(tocReader).Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(Foundation, Foundation_IO_Archive_Implementation_ArchiveUtils);
