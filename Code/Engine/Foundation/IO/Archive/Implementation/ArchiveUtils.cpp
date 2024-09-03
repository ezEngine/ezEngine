#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Archive/ArchiveUtils.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/IO/CompressedStreamZlib.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>

ezHybridArray<ezString, 4, ezStaticsAllocatorWrapper>& ezArchiveUtils::GetAcceptedArchiveFileExtensions()
{
  static ezHybridArray<ezString, 4, ezStaticsAllocatorWrapper> extensions;

  if (extensions.IsEmpty())
  {
    extensions.PushBack("ezArchive");
  }

  return extensions;
}

bool ezArchiveUtils::IsAcceptedArchiveFileExtensions(ezStringView sExtension)
{
  for (const auto& ext : GetAcceptedArchiveFileExtensions())
  {
    if (sExtension.IsEqual_NoCase(ext.GetView()))
      return true;
  }

  return false;
}

ezResult ezArchiveUtils::WriteHeader(ezStreamWriter& inout_stream)
{
  static_assert(16 == ArchiveHeaderSize);

  const char* szTag = "EZARCHIVE";
  EZ_SUCCEED_OR_RETURN(inout_stream.WriteBytes(szTag, 10));

  const ezUInt8 uiArchiveVersion = 4;

  // Version 2: Added end-of-file marker for file corruption (cutoff) detection
  // Version 3: HashedStrings changed from MurmurHash to xxHash
  // Version 4: use 64 Bit string hashes
  inout_stream << uiArchiveVersion;

  const ezUInt8 uiPadding[5] = {0, 0, 0, 0, 0};
  EZ_SUCCEED_OR_RETURN(inout_stream.WriteBytes(uiPadding, 5));

  return EZ_SUCCESS;
}

ezResult ezArchiveUtils::ReadHeader(ezStreamReader& inout_stream, ezUInt8& out_uiVersion)
{
  static_assert(16 == ArchiveHeaderSize);

  char szTag[10];
  if (inout_stream.ReadBytes(szTag, 10) != 10 || !ezStringUtils::IsEqual(szTag, "EZARCHIVE"))
  {
    ezLog::Error("Invalid or corrupted archive. Archive-marker not found.");
    return EZ_FAILURE;
  }

  out_uiVersion = 0;
  inout_stream >> out_uiVersion;

  if (out_uiVersion != 1 && out_uiVersion != 2 && out_uiVersion != 3 && out_uiVersion != 4)
  {
    ezLog::Error("Unsupported archive version '{}'.", out_uiVersion);
    return EZ_FAILURE;
  }

  ezUInt8 uiPadding[5] = {255, 255, 255, 255, 255};
  if (inout_stream.ReadBytes(uiPadding, 5) != 5)
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

ezResult ezArchiveUtils::WriteEntryPreprocessed(ezStreamWriter& inout_stream, ezConstByteArrayPtr entryData, ezUInt32 uiPathStringOffset, ezArchiveCompressionMode compression, ezUInt32 uiUncompressedEntryDataSize, ezArchiveEntry& ref_tocEntry, ezUInt64& inout_uiCurrentStreamPosition)
{
  EZ_SUCCEED_OR_RETURN(inout_stream.WriteBytes(entryData.GetPtr(), entryData.GetCount()));

  ref_tocEntry.m_uiPathStringOffset = uiPathStringOffset;
  ref_tocEntry.m_uiDataStartOffset = inout_uiCurrentStreamPosition;
  ref_tocEntry.m_uiUncompressedDataSize = uiUncompressedEntryDataSize;
  ref_tocEntry.m_uiStoredDataSize = entryData.GetCount();
  ref_tocEntry.m_CompressionMode = compression;

  inout_uiCurrentStreamPosition += entryData.GetCount();

  return EZ_SUCCESS;
}

ezResult ezArchiveUtils::WriteEntry(
  ezStreamWriter& inout_stream, ezStringView sAbsSourcePath, ezUInt32 uiPathStringOffset, ezArchiveCompressionMode compression,
  ezInt32 iCompressionLevel, ezArchiveEntry& inout_tocEntry, ezUInt64& inout_uiCurrentStreamPosition, FileWriteProgressCallback progress /*= FileWriteProgressCallback()*/)
{
  EZ_IGNORE_UNUSED(iCompressionLevel);

  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(sAbsSourcePath, 1024 * 1024));

  const ezUInt64 uiMaxBytes = file.GetFileSize();

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  constexpr ezUInt32 uiMaxNumWorkerThreads = 12u;

  ezUInt32 uiWorkerThreadCount;
  if (uiMaxBytes > ezMath::MaxValue<ezUInt32>())
  {
    uiWorkerThreadCount = uiMaxNumWorkerThreads;
  }
  else
  {
    constexpr ezUInt32 uiBytesPerThread = 1024u * 1024u;
    uiWorkerThreadCount = ezMath::Clamp((ezUInt32)floor(uiMaxBytes / uiBytesPerThread), 1u, uiMaxNumWorkerThreads);
  }
#endif

  inout_tocEntry.m_uiPathStringOffset = uiPathStringOffset;
  inout_tocEntry.m_uiDataStartOffset = inout_uiCurrentStreamPosition;
  inout_tocEntry.m_uiUncompressedDataSize = 0;

  ezStreamWriter* pWriter = &inout_stream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  ezCompressedStreamWriterZstd zstdWriter;
#endif

  switch (compression)
  {
    case ezArchiveCompressionMode::Uncompressed:
      break;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    case ezArchiveCompressionMode::Compressed_zstd:
    {
      zstdWriter.SetOutputStream(&inout_stream, uiWorkerThreadCount, (ezCompressedStreamWriterZstd::Compression)iCompressionLevel);
      pWriter = &zstdWriter;
    }
    break;
#endif

    default:
      compression = ezArchiveCompressionMode::Uncompressed;
      break;
  }

  inout_tocEntry.m_CompressionMode = compression;

  ezUInt64 uiRead = 0;
  ezDynamicArray<ezUInt8> buf;
  buf.SetCountUninitialized(1024 * 32);
  while (true)
  {
    uiRead = file.ReadBytes(buf.GetData(), buf.GetCount());

    if (uiRead == 0)
      break;

    inout_tocEntry.m_uiUncompressedDataSize += uiRead;

    if (progress.IsValid())
    {
      if (!progress(inout_tocEntry.m_uiUncompressedDataSize, uiMaxBytes))
        return EZ_FAILURE;
    }

    EZ_SUCCEED_OR_RETURN(pWriter->WriteBytes(buf.GetData(), uiRead));
  }


  switch (compression)
  {
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    case ezArchiveCompressionMode::Compressed_zstd:
      EZ_SUCCEED_OR_RETURN(zstdWriter.FinishCompressedStream());
      inout_tocEntry.m_uiStoredDataSize = zstdWriter.GetWrittenBytes();
      break;
#endif

    case ezArchiveCompressionMode::Uncompressed:
    default:
      inout_tocEntry.m_uiStoredDataSize = inout_tocEntry.m_uiUncompressedDataSize;
      break;
  }

  inout_uiCurrentStreamPosition += inout_tocEntry.m_uiStoredDataSize;

  return EZ_SUCCESS;
}

ezResult ezArchiveUtils::WriteEntryOptimal(ezStreamWriter& inout_stream, ezStringView sAbsSourcePath, ezUInt32 uiPathStringOffset, ezArchiveCompressionMode compression, ezInt32 iCompressionLevel, ezArchiveEntry& ref_tocEntry, ezUInt64& inout_uiCurrentStreamPosition, FileWriteProgressCallback progress /*= FileWriteProgressCallback()*/)
{
  if (compression == ezArchiveCompressionMode::Uncompressed)
  {
    return WriteEntry(inout_stream, sAbsSourcePath, uiPathStringOffset, ezArchiveCompressionMode::Uncompressed, iCompressionLevel, ref_tocEntry, inout_uiCurrentStreamPosition, progress);
  }
  else
  {
    ezDefaultMemoryStreamStorage storage;
    ezMemoryStreamWriter writer(&storage);

    ezUInt64 streamPos = inout_uiCurrentStreamPosition;
    EZ_SUCCEED_OR_RETURN(WriteEntry(writer, sAbsSourcePath, uiPathStringOffset, compression, iCompressionLevel, ref_tocEntry, streamPos, progress));

    if (ref_tocEntry.m_uiStoredDataSize * 12 >= ref_tocEntry.m_uiUncompressedDataSize * 10)
    {
      // less than 20% size saving -> go uncompressed
      return WriteEntry(inout_stream, sAbsSourcePath, uiPathStringOffset, ezArchiveCompressionMode::Uncompressed, iCompressionLevel, ref_tocEntry, inout_uiCurrentStreamPosition, progress);
    }
    else
    {
      auto res = storage.CopyToStream(inout_stream);
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

void ezArchiveUtils::ConfigureRawMemoryStreamReader(const ezArchiveEntry& entry, const void* pStartOfArchiveData, ezRawMemoryStreamReader& ref_memReader)
{
  ref_memReader.Reset(ezMemoryUtils::AddByteOffset(pStartOfArchiveData, static_cast<std::ptrdiff_t>(entry.m_uiDataStartOffset)), entry.m_uiStoredDataSize);
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

ezResult ezArchiveUtils::AppendTOC(ezStreamWriter& inout_stream, const ezArchiveTOC& toc)
{
  ezDefaultMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);

  EZ_SUCCEED_OR_RETURN(toc.Serialize(writer));

  EZ_SUCCEED_OR_RETURN(storage.CopyToStream(inout_stream));

  TocMetaData tocMeta;

  ezHashStreamWriter64 hashStream(tocMeta.m_uiSize);
  EZ_SUCCEED_OR_RETURN(storage.CopyToStream(hashStream));

  // Added in file version 2: hash of the TOC
  tocMeta.m_uiSize = storage.GetStorageSize32();
  tocMeta.m_uiHash = hashStream.GetHashValue();

  // append the TOC meta data
  inout_stream << tocMeta.m_uiSize;
  inout_stream << tocMeta.m_uiHash;

  // write an 'end' marker
  return inout_stream.WriteBytes(szEndMarker, 14);
}

static ezResult VerifyEndMarker(ezUInt64 uiArchiveDataSize, const void* pArchiveDataBuffer, ezUInt8 uiArchiveVersion)
{
  const ezUInt32 uiEndMarkerSize = GetEndMarkerSize(uiArchiveVersion);

  if (uiEndMarkerSize == 0)
  {
    return EZ_SUCCESS;
  }

  if (uiEndMarkerSize > uiArchiveDataSize)
  {
    ezLog::Error("Archive is too small. End-marker not found.");
    return EZ_FAILURE;
  }

  const void* pStart = ezMemoryUtils::AddByteOffset(pArchiveDataBuffer, uiArchiveDataSize - uiEndMarkerSize);

  ezRawMemoryStreamReader reader(pStart, uiEndMarkerSize);

  char szMarker[32] = "";
  if (reader.ReadBytes(szMarker, uiEndMarkerSize) != uiEndMarkerSize || !ezStringUtils::IsEqual(szMarker, szEndMarker))
  {
    ezLog::Error("Archive is corrupt or cut off. End-marker not found.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezArchiveUtils::ExtractTOCMeta(ezUInt64 uiArchiveEndingDataSize, const void* pArchiveEndingDataBuffer, TOCMeta& ref_tocMeta, ezUInt8 uiArchiveVersion)
{
  EZ_SUCCEED_OR_RETURN(VerifyEndMarker(uiArchiveEndingDataSize, pArchiveEndingDataBuffer, uiArchiveVersion));

  const ezUInt32 uiEndMarkerSize = GetEndMarkerSize(uiArchiveVersion);
  const ezUInt32 uiTocMetaSize = GetTocMetaSize(uiArchiveVersion);

  ezUInt32 uiTocSize = 0;
  ezUInt64 uiExpectedTocHash = 0;

  // read the TOC meta data
  {
    EZ_ASSERT_DEV(uiEndMarkerSize + uiTocMetaSize <= ArchiveTOCMetaMaxFooterSize, "");

    if (uiEndMarkerSize + uiTocMetaSize > uiArchiveEndingDataSize)
    {
      ezLog::Error("Unable to extract Archive TOC. File size too small: {0}", ezArgFileSize(uiArchiveEndingDataSize));
      return EZ_FAILURE;
    }

    const void* pTocMetaStart = ezMemoryUtils::AddByteOffset(pArchiveEndingDataBuffer, uiArchiveEndingDataSize - uiEndMarkerSize - uiTocMetaSize);

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

  // output the result
  {
    ref_tocMeta = TOCMeta();
    ref_tocMeta.m_uiTocSize = uiTocSize;
    ref_tocMeta.m_uiExpectedTocHash = uiExpectedTocHash;
    ref_tocMeta.m_uiTocOffsetFromArchiveEnd = uiTocSize + uiTocMetaSize + uiEndMarkerSize;
  }

  return EZ_SUCCESS;
}

ezResult ezArchiveUtils::ExtractTOCMeta(const ezMemoryMappedFile& memFile, TOCMeta& ref_tocMeta, ezUInt8 uiArchiveVersion)
{
  return ExtractTOCMeta(memFile.GetFileSize(), memFile.GetReadPointer(), ref_tocMeta, uiArchiveVersion);
}

ezResult ezArchiveUtils::ExtractTOC(ezUInt64 uiArchiveEndingDataSize, const void* pArchiveEndingDataBuffer, ezArchiveTOC& ref_toc, ezUInt8 uiArchiveVersion)
{
  // get toc meta
  TOCMeta tocMeta;
  if (ExtractTOCMeta(uiArchiveEndingDataSize, pArchiveEndingDataBuffer, tocMeta, uiArchiveVersion).Failed())
  {
    return EZ_FAILURE;
  }

  // verify meta is valid
  if (tocMeta.m_uiTocOffsetFromArchiveEnd > uiArchiveEndingDataSize)
  {
    ezLog::Error("Archive TOC offset is corrupted.");
    return EZ_FAILURE;
  }

  // get toc data ptr
  const void* pTocStart = ezMemoryUtils::AddByteOffset(pArchiveEndingDataBuffer, uiArchiveEndingDataSize - tocMeta.m_uiTocOffsetFromArchiveEnd);

  // validate the TOC hash
  if (uiArchiveVersion >= 2)
  {
    const ezUInt64 uiActualTocHash = ezHashingUtils::xxHash64(pTocStart, tocMeta.m_uiTocSize);
    if (tocMeta.m_uiExpectedTocHash != uiActualTocHash)
    {
      ezLog::Error("Archive TOC is corrupted. Hashes do not match.");
      return EZ_FAILURE;
    }
  }

  // read the actual TOC data
  {
    ezRawMemoryStreamReader tocReader(pTocStart, tocMeta.m_uiTocSize);

    if (ref_toc.Deserialize(tocReader, uiArchiveVersion).Failed())
    {
      ezLog::Error("Failed to deserialize ezArchive TOC");
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezArchiveUtils::ExtractTOC(const ezMemoryMappedFile& memFile, ezArchiveTOC& ref_toc, ezUInt8 uiArchiveVersion)
{
  return ExtractTOC(memFile.GetFileSize(), memFile.GetReadPointer(), ref_toc, uiArchiveVersion);
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

  ezStreamReader& operator>>(ezStreamReader& inout_stream, EndOfCDHeader& ref_value)
  {
    inout_stream >> ref_value.signature >> ref_value.diskNumber >> ref_value.diskWithCD >> ref_value.diskEntries >> ref_value.totalEntries >> ref_value.cdSize;
    inout_stream >> ref_value.cdOffset >> ref_value.commentLength;
    EZ_ASSERT_DEBUG(ref_value.signature == EndOfCDMagicSignature, "ZIP: Corrupt end of central directory header.");
    return inout_stream;
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

  ezStreamReader& operator>>(ezStreamReader& inout_stream, CDFileHeader& ref_value)
  {
    inout_stream >> ref_value.signature >> ref_value.version >> ref_value.versionNeeded >> ref_value.flags >> ref_value.compression >> ref_value.modTime >> ref_value.modDate;
    inout_stream >> ref_value.crc32 >> ref_value.compressedSize >> ref_value.uncompressedSize >> ref_value.fileNameLength >> ref_value.extraFieldLength;
    inout_stream >> ref_value.fileCommentLength >> ref_value.diskNumStart >> ref_value.internalAttr >> ref_value.externalAttr >> ref_value.offsetLocalHeader;
    EZ_IGNORE_UNUSED(CDFileMagicSignature);
    EZ_ASSERT_DEBUG(ref_value.signature == CDFileMagicSignature, "ZIP: Corrupt central directory file entry header.");
    return inout_stream;
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

  ezStreamReader& operator>>(ezStreamReader& inout_stream, LocalFileHeader& ref_value)
  {
    inout_stream >> ref_value.signature >> ref_value.version >> ref_value.flags >> ref_value.compression >> ref_value.modTime >> ref_value.modDate >> ref_value.crc32;
    inout_stream >> ref_value.compressedSize >> ref_value.uncompressedSize >> ref_value.fileNameLength >> ref_value.extraFieldLength;
    EZ_ASSERT_DEBUG(ref_value.signature == LocalFileMagicSignature, "ZIP: Corrupt local file entry header.");
    return inout_stream;
  }
}; // namespace ZipFormat

ezResult ezArchiveUtils::ReadZipHeader(ezStreamReader& inout_stream, ezUInt8& out_uiVersion)
{
  using namespace ZipFormat;

  ezUInt32 header;
  inout_stream >> header;
  if (header == LocalFileMagicSignature)
  {
    out_uiVersion = 0;
    return EZ_SUCCESS;
  }
  return EZ_SUCCESS;
}

ezResult ezArchiveUtils::ExtractZipTOC(const ezMemoryMappedFile& memFile, ezArchiveTOC& ref_toc)
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

  ref_toc.m_Entries.Reserve(ecdHeader.diskEntries);
  ref_toc.m_PathToEntryIndex.Reserve(ecdHeader.diskEntries);

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
      auto& entry = ref_toc.m_Entries.ExpandAndGetRef();
      entry.m_uiUncompressedDataSize = cdfHeader.uncompressedSize;
      entry.m_uiStoredDataSize = cdfHeader.compressedSize;
      entry.m_uiPathStringOffset = ref_toc.m_AllPathStrings.GetCount();
      entry.m_CompressionMode = cdfHeader.compression == CompressionType::Uncompressed ? ezArchiveCompressionMode::Uncompressed : ezArchiveCompressionMode::Compressed_zip;

      auto nameBuffer = ezArrayPtr<const ezUInt8>(static_cast<const ezUInt8*>(pCdfStart) + CDFileHeaderLength, cdfHeader.fileNameLength);
      ref_toc.m_AllPathStrings.PushBackRange(nameBuffer);
      ref_toc.m_AllPathStrings.PushBack(0);
      const char* szName = reinterpret_cast<const char*>(ref_toc.m_AllPathStrings.GetData() + entry.m_uiPathStringOffset);
      sLowerCaseHash = szName;
      sLowerCaseHash.ToLower();
      ref_toc.m_PathToEntryIndex.Insert(ezArchiveStoredString(ezHashingUtils::StringHash(sLowerCaseHash), entry.m_uiPathStringOffset), ref_toc.m_Entries.GetCount() - 1);

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


