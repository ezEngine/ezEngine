#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Archive/ArchiveUtils.h>

#include <Foundation/IO/CompressedStreamZlib.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>

ezHybridArray<ezString, 4, ezStaticAllocatorWrapper>& ezArchiveUtils::GetAcceptedArchiveFileExtensions()
{
  static ezHybridArray<ezString, 4, ezStaticAllocatorWrapper> extensions;

  if (extensions.IsEmpty())
  {
    extensions.PushBack("ezArchive");
  }

  return extensions;
}

bool ezArchiveUtils::IsAcceptedArchiveFileExtensions(ezStringView extension)
{
  for (const auto& ext : GetAcceptedArchiveFileExtensions())
  {
    if (extension.IsEqual_NoCase(ext.GetView()))
      return true;
  }

  return false;
}

ezResult ezArchiveUtils::WriteHeader(ezStreamWriter& stream)
{
  const char* szTag = "EZARCHIVE";
  EZ_SUCCEED_OR_RETURN(stream.WriteBytes(szTag, 10));

  const ezUInt8 uiArchiveVersion = 4;

  // Version 2: Added end-of-file marker for file corruption (cutoff) detection
  // Version 3: HashedStrings changed from MurmurHash to xxHash
  // Version 4: use 64 Bit string hashes
  stream << uiArchiveVersion;

  const ezUInt8 uiPadding[5] = {0, 0, 0, 0, 0};
  EZ_SUCCEED_OR_RETURN(stream.WriteBytes(uiPadding, 5));

  return EZ_SUCCESS;
}

ezResult ezArchiveUtils::ReadHeader(ezStreamReader& stream, ezUInt8& out_uiVersion)
{
  char szTag[10];
  if (stream.ReadBytes(szTag, 10) != 10 || !ezStringUtils::IsEqual(szTag, "EZARCHIVE"))
  {
    ezLog::Error("Invalid or corrupted archive. Archive-marker not found.");
    return EZ_FAILURE;
  }

  out_uiVersion = 0;
  stream >> out_uiVersion;

  if (out_uiVersion != 1 && out_uiVersion != 2 && out_uiVersion != 3 && out_uiVersion != 4)
  {
    ezLog::Error("Unsupported archive version '{}'.", out_uiVersion);
    return EZ_FAILURE;
  }

  ezUInt8 uiPadding[5] = {255, 255, 255, 255, 255};
  if (stream.ReadBytes(uiPadding, 5) != 5)
  {
    ezLog::Error("Invalid or corrupted archive. Missing header data.");
    return EZ_FAILURE;
  }

  const ezUInt8 uiZeroPadding[5] = {0, 0, 0, 0, 0};

  if (ezMemoryUtils::Compare<ezUInt8>(uiPadding, uiZeroPadding, 5) != 0)
  {
    ezLog::Error("Invalid or corrupted archive. Unexpected header data.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezArchiveUtils::WriteEntry(ezStreamWriter& stream, const char* szAbsSourcePath, ezUInt32 uiPathStringOffset, ezArchiveCompressionMode compression, ezArchiveEntry& tocEntry, ezUInt64& inout_uiCurrentStreamPosition, FileWriteProgressCallback progress /*= FileWriteProgressCallback()*/)
{
  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(szAbsSourcePath, 1024 * 1024));

  const ezUInt64 uiMaxBytes = file.GetFileSize();

  ezUInt8 uiTemp[1024 * 8];

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

ezResult ezArchiveUtils::WriteEntryOptimal(ezStreamWriter& stream, const char* szAbsSourcePath, ezUInt32 uiPathStringOffset, ezArchiveCompressionMode compression, ezArchiveEntry& tocEntry, ezUInt64& inout_uiCurrentStreamPosition, FileWriteProgressCallback progress /*= FileWriteProgressCallback()*/)
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
      auto res = stream.WriteBytes(storage.GetData(), storage.GetStorageSize());
      inout_uiCurrentStreamPosition = streamPos;

      return res;
    }
  }
}

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT

class ezCompressedStreamReaderZstdWithSource : public ezCompressedStreamReaderZstd
{
public:
  ezRawMemoryStreamReader m_Source;
};

#endif

#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT

class ezCompressedStreamReaderZipWithSource : public ezCompressedStreamReaderZip
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
      reader = EZ_DEFAULT_NEW(ezCompressedStreamReaderZstdWithSource);
      ezCompressedStreamReaderZstdWithSource* pRawReader = static_cast<ezCompressedStreamReaderZstdWithSource*>(reader.Borrow());
      ConfigureRawMemoryStreamReader(entry, pStartOfArchiveData, pRawReader->m_Source);
      pRawReader->SetInputStream(&pRawReader->m_Source);
      break;
    }
#endif
#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
    case ezArchiveCompressionMode::Compressed_zip:
    {
      reader = EZ_DEFAULT_NEW(ezCompressedStreamReaderZipWithSource);
      ezCompressedStreamReaderZipWithSource* pRawReader = static_cast<ezCompressedStreamReaderZipWithSource*>(reader.Borrow());
      ConfigureRawMemoryStreamReader(entry, pStartOfArchiveData, pRawReader->m_Source);
      pRawReader->SetInputStream(&pRawReader->m_Source, entry.m_uiStoredDataSize);
      break;
    }
#endif

    default:
      EZ_REPORT_FAILURE("Archive entry compression mode '{}' is not supported by ezArchiveReader", (int)entry.m_CompressionMode);
      break;
  }

  return std::move(reader);
}

void ezArchiveUtils::ConfigureRawMemoryStreamReader(const ezArchiveEntry& entry, const void* pStartOfArchiveData, ezRawMemoryStreamReader& memReader)
{
  memReader.Reset(ezMemoryUtils::AddByteOffset(pStartOfArchiveData, static_cast<ptrdiff_t>(entry.m_uiDataStartOffset)), entry.m_uiStoredDataSize);
}

static const char* szEndMarker = "EZARCHIVE-END";

static ezUInt32 GetEndMarkerSize(ezUInt8 uiFileVersion)
{
  if (uiFileVersion == 1)
    return 0;

  return 14;
}

static ezUInt32 GetTocMetaSize(ezUInt8 uiFileVersion)
{
  if (uiFileVersion == 1)
    return sizeof(ezUInt32); // TOC size

  return sizeof(ezUInt32) /* TOC size */ + sizeof(ezUInt64) /* TOC hash */;
}

struct TocMetaData
{
  ezUInt32 m_uiSize = 0;
  ezUInt64 m_uiHash = 0;
};

ezResult ezArchiveUtils::AppendTOC(ezStreamWriter& stream, const ezArchiveTOC& toc)
{
  ezMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);

  EZ_SUCCEED_OR_RETURN(toc.Serialize(writer));

  EZ_SUCCEED_OR_RETURN(stream.WriteBytes(storage.GetData(), storage.GetStorageSize()));

  TocMetaData tocMeta;

  // Added in file version 2: hash of the TOC
  tocMeta.m_uiSize = storage.GetStorageSize();
  tocMeta.m_uiHash = ezHashingUtils::xxHash64(storage.GetData(), tocMeta.m_uiSize);

  // append the TOC meta data
  stream << tocMeta.m_uiSize;
  stream << tocMeta.m_uiHash;

  // write an 'end' marker
  return stream.WriteBytes(szEndMarker, 14);
}

static ezResult VerifyEndMarker(ezMemoryMappedFile& memFile, ezUInt8 uiArchiveVersion)
{
  const ezUInt32 uiEndMarkerSize = GetEndMarkerSize(uiArchiveVersion);

  if (uiEndMarkerSize == 0)
    return EZ_SUCCESS;

  const void* pStart = memFile.GetReadPointer(uiEndMarkerSize, ezMemoryMappedFile::OffsetBase::End);

  ezRawMemoryStreamReader reader(pStart, uiEndMarkerSize);

  char szMarker[32] = "";
  if (reader.ReadBytes(szMarker, uiEndMarkerSize) != uiEndMarkerSize || !ezStringUtils::IsEqual(szMarker, szEndMarker))
  {
    ezLog::Error("Archive is corrupt or cut off. End-marker not found.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezArchiveUtils::ExtractTOC(ezMemoryMappedFile& memFile, ezArchiveTOC& toc, ezUInt8 uiArchiveVersion)
{
  EZ_SUCCEED_OR_RETURN(VerifyEndMarker(memFile, uiArchiveVersion));

  const ezUInt32 uiEndMarkerSize = GetEndMarkerSize(uiArchiveVersion);
  const ezUInt32 uiTocMetaSize = GetTocMetaSize(uiArchiveVersion);

  ezUInt32 uiTocSize = 0;
  ezUInt64 uiExpectedTocHash = 0;

  // read the TOC meta data
  {
    const void* pTocMetaStart = memFile.GetReadPointer(uiEndMarkerSize + uiTocMetaSize, ezMemoryMappedFile::OffsetBase::End);

    ezRawMemoryStreamReader tocMetaReader(pTocMetaStart, uiTocMetaSize);

    tocMetaReader >> uiTocSize;

    if (uiTocSize > 1024 * 1024 * 1024) // 1GB of TOC is enough for ~16M entries...
    {
      ezLog::Error("Archive TOC is probably corrupted. Unreasonable TOC size: {0}", ezArgFileSize(uiTocSize));
      return EZ_FAILURE;
    }

    if (uiArchiveVersion >= 2)
    {
      tocMetaReader >> uiExpectedTocHash;
    }
  }

  const void* pTocStart = memFile.GetReadPointer(uiTocSize + uiTocMetaSize + uiEndMarkerSize, ezMemoryMappedFile::OffsetBase::End);

  // validate the TOC hash
  if (uiArchiveVersion >= 2)
  {
    const ezUInt64 uiActualTocHash = ezHashingUtils::xxHash64(pTocStart, uiTocSize);
    if (uiExpectedTocHash != uiActualTocHash)
    {
      ezLog::Error("Archive TOC is corrupted. Hashes do not match.");
      return EZ_FAILURE;
    }
  }

  // read the actual TOC data
  {
    ezRawMemoryStreamReader tocReader(pTocStart, uiTocSize);

    if (toc.Deserialize(tocReader, uiArchiveVersion).Failed())
    {
      ezLog::Error("Failed to deserialize ezArchive TOC");
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

namespace ZipFormat
{
  constexpr ezUInt32 EndOfCDMagicSignature = 0x06054b50;
  constexpr ezUInt32 EndOfCDHeaderLength = 22;

  constexpr ezUInt32 MaxCommentLength = 65535;
  constexpr ezUInt64 MaxEndOfCDSearchLength = MaxCommentLength + EndOfCDHeaderLength;

  constexpr ezUInt32 LocalFileMagicSignature = 0x04034b50;
  constexpr ezUInt32 LocalFileHeaderLength = 30;

  constexpr ezUInt32 CDFileMagicSignature = 0x02014b50;
  constexpr ezUInt32 CDFileHeaderLength = 46;

  enum CompressionType
  {
    Uncompressed = 0,
    Deflate = 8,

  };

  struct EndOfCDHeader
  {
    ezUInt32 signature;
    ezUInt16 diskNumber;
    ezUInt16 diskWithCD;
    ezUInt16 diskEntries;
    ezUInt16 totalEntries;
    ezUInt32 cdSize;
    ezUInt32 cdOffset;
    ezUInt16 commentLength;
  };

  ezStreamReader& operator>>(ezStreamReader& Stream, EndOfCDHeader& Value)
  {
    Stream >> Value.signature >> Value.diskNumber >> Value.diskWithCD >> Value.diskEntries >> Value.totalEntries >> Value.cdSize;
    Stream >> Value.cdOffset >> Value.commentLength;
    EZ_ASSERT_DEBUG(Value.signature == EndOfCDMagicSignature, "ZIP: Corrupt end of central directory header.");
    return Stream;
  }

  struct CDFileHeader
  {
    ezUInt32 signature;
    ezUInt16 version;
    ezUInt16 versionNeeded;
    ezUInt16 flags;
    ezUInt16 compression;
    ezUInt16 modTime;
    ezUInt16 modDate;
    ezUInt32 crc32;
    ezUInt32 compressedSize;
    ezUInt32 uncompressedSize;
    ezUInt16 fileNameLength;
    ezUInt16 extraFieldLength;
    ezUInt16 fileCommentLength;
    ezUInt16 diskNumStart;
    ezUInt16 internalAttr;
    ezUInt32 externalAttr;
    ezUInt32 offsetLocalHeader;
  };

  ezStreamReader& operator>>(ezStreamReader& Stream, CDFileHeader& Value)
  {
    Stream >> Value.signature >> Value.version >> Value.versionNeeded >> Value.flags >> Value.compression >> Value.modTime >> Value.modDate;
    Stream >> Value.crc32 >> Value.compressedSize >> Value.uncompressedSize >> Value.fileNameLength >> Value.extraFieldLength;
    Stream >> Value.fileCommentLength >> Value.diskNumStart >> Value.internalAttr >> Value.externalAttr >> Value.offsetLocalHeader;
    EZ_ASSERT_DEBUG(Value.signature == CDFileMagicSignature, "ZIP: Corrupt central directory file entry header.");
    return Stream;
  }

  struct LocalFileHeader
  {
    ezUInt32 signature;
    ezUInt16 version;
    ezUInt16 flags;
    ezUInt16 compression;
    ezUInt16 modTime;
    ezUInt16 modDate;
    ezUInt32 crc32;
    ezUInt32 compressedSize;
    ezUInt32 uncompressedSize;
    ezUInt16 fileNameLength;
    ezUInt16 extraFieldLength;
  };

  ezStreamReader& operator>>(ezStreamReader& Stream, LocalFileHeader& Value)
  {
    Stream >> Value.signature >> Value.version >> Value.flags >> Value.compression >> Value.modTime >> Value.modDate >> Value.crc32;
    Stream >> Value.compressedSize >> Value.uncompressedSize >> Value.fileNameLength >> Value.extraFieldLength;
    EZ_ASSERT_DEBUG(Value.signature == LocalFileMagicSignature, "ZIP: Corrupt local file entry header.");
    return Stream;
  }
}; // namespace ZipFormat

ezResult ezArchiveUtils::ReadZipHeader(ezStreamReader& stream, ezUInt8& out_uiVersion)
{
  using namespace ZipFormat;

  ezUInt32 header;
  stream >> header;
  if (header == LocalFileMagicSignature)
  {
    out_uiVersion = 0;
    return EZ_SUCCESS;
  }
  return EZ_SUCCESS;
}

ezResult ezArchiveUtils::ExtractZipTOC(ezMemoryMappedFile& memFile, ezArchiveTOC& toc)
{
  using namespace ZipFormat;

  const ezUInt8* pEndOfCDStart = nullptr;
  {
    // Find End of CD signature by searching from the end of the file.
    // As a comment can come after it we have to potentially walk max comment length backwards.
    const ezUInt64 SearchEnd = memFile.GetFileSize() - ezMath::Min(MaxEndOfCDSearchLength, memFile.GetFileSize());
    const ezUInt8* pSearchEnd = static_cast<const ezUInt8*>(memFile.GetReadPointer(SearchEnd, ezMemoryMappedFile::OffsetBase::End));
    const ezUInt8* pSearchStart = static_cast<const ezUInt8*>(memFile.GetReadPointer(EndOfCDHeaderLength, ezMemoryMappedFile::OffsetBase::End));
    while (pSearchStart >= pSearchEnd)
    {
      if (*reinterpret_cast<const ezUInt32*>(pSearchStart) == EndOfCDMagicSignature)
      {
        pEndOfCDStart = pSearchStart;
        break;
      }
      pSearchStart--;
    }
    if (pEndOfCDStart == nullptr)
      return EZ_FAILURE;
  }

  ezRawMemoryStreamReader tocReader(pEndOfCDStart, EndOfCDHeaderLength);
  EndOfCDHeader ecdHeader;
  tocReader >> ecdHeader;

  toc.m_Entries.Reserve(ecdHeader.diskEntries);
  toc.m_PathToEntryIndex.Reserve(ecdHeader.diskEntries);

  ezStringBuilder sLowerCaseHash;
  ezUInt64 uiEntryOffset = 0;
  for (ezUInt16 uiEntry = 0; uiEntry < ecdHeader.diskEntries; ++uiEntry)
  {
    // First, read the current file's header from the central directory
    const void* pCdfStart = memFile.GetReadPointer(ecdHeader.cdOffset + uiEntryOffset, ezMemoryMappedFile::OffsetBase::Start);
    ezRawMemoryStreamReader cdfReader(pCdfStart, ecdHeader.cdSize - uiEntryOffset);
    CDFileHeader cdfHeader;
    cdfReader >> cdfHeader;

    if (cdfHeader.compression == CompressionType::Uncompressed || cdfHeader.compression == CompressionType::Deflate)
    {
      auto& entry = toc.m_Entries.ExpandAndGetRef();
      entry.m_uiUncompressedDataSize = cdfHeader.uncompressedSize;
      entry.m_uiStoredDataSize = cdfHeader.compressedSize;
      entry.m_uiPathStringOffset = toc.m_AllPathStrings.GetCount();
      entry.m_CompressionMode = cdfHeader.compression == CompressionType::Uncompressed ? ezArchiveCompressionMode::Uncompressed : ezArchiveCompressionMode::Compressed_zip;

      auto nameBuffer = ezArrayPtr<const ezUInt8>(static_cast<const ezUInt8*>(pCdfStart) + CDFileHeaderLength, cdfHeader.fileNameLength);
      toc.m_AllPathStrings.PushBackRange(nameBuffer);
      toc.m_AllPathStrings.PushBack(0);
      const char* szName = reinterpret_cast<const char*>(toc.m_AllPathStrings.GetData() + entry.m_uiPathStringOffset);
      sLowerCaseHash = szName;
      sLowerCaseHash.ToLower();
      toc.m_PathToEntryIndex.Insert(ezArchiveStoredString(ezHashingUtils::StringHash(sLowerCaseHash.GetData()), entry.m_uiPathStringOffset), toc.m_Entries.GetCount() - 1);

      // Compute data stream start location. We need to skip past the local (and redundant) file header to find it.
      const void* pLfStart = memFile.GetReadPointer(cdfHeader.offsetLocalHeader, ezMemoryMappedFile::OffsetBase::Start);
      ezRawMemoryStreamReader lfReader(pLfStart, memFile.GetFileSize() - cdfHeader.offsetLocalHeader);
      LocalFileHeader lfHeader;
      lfReader >> lfHeader;
      entry.m_uiDataStartOffset = cdfHeader.offsetLocalHeader + LocalFileHeaderLength + lfHeader.fileNameLength + lfHeader.extraFieldLength;
    }
    // Compute next file header location.
    uiEntryOffset += CDFileHeaderLength + cdfHeader.fileNameLength + cdfHeader.extraFieldLength + cdfHeader.fileCommentLength;
  }

  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(Foundation, Foundation_IO_Archive_Implementation_ArchiveUtils);
