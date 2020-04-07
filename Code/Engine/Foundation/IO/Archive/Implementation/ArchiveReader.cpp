#include <FoundationPCH.h>

#include <Foundation/IO/Archive/ArchiveReader.h>
#include <Foundation/IO/Archive/ArchiveUtils.h>

#include <Foundation/IO/Archive/ArchiveUtils.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Types/Types.h>

#include <Foundation/Logging/Log.h>

ezResult ezArchiveReader::OpenArchive(const char* szPath)
{
#if EZ_ENABLED(EZ_SUPPORTS_MEMORY_MAPPED_FILE)
  EZ_LOG_BLOCK("OpenArchive", szPath);

  EZ_SUCCEED_OR_RETURN(m_MemFile.Open(szPath, ezMemoryMappedFile::Mode::ReadOnly));
  m_uiMemFileSize = m_MemFile.GetFileSize();

  // validate the archive
  {
    ezRawMemoryStreamReader reader(m_MemFile.GetReadPointer(), m_MemFile.GetFileSize());

    ezStringView extension = ezPathUtils::GetFileExtension(szPath);

    if (ezArchiveUtils::IsAcceptedArchiveFileExtensions(extension))
    {
      EZ_SUCCEED_OR_RETURN(ezArchiveUtils::ReadHeader(reader, m_uiArchiveVersion));

      m_pDataStart = m_MemFile.GetReadPointer(16, ezMemoryMappedFile::OffsetBase::Start);

      EZ_SUCCEED_OR_RETURN(ezArchiveUtils::ExtractTOC(m_MemFile, m_ArchiveTOC, m_uiArchiveVersion));
    }
#  ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
    else if (extension == "zip" || extension == "apk")
    {
      EZ_SUCCEED_OR_RETURN(ezArchiveUtils::ReadZipHeader(reader, m_uiArchiveVersion));
      if (m_uiArchiveVersion != 0)
      {
        ezLog::Error("Unknown zip version '{}'", m_uiArchiveVersion);
        return EZ_FAILURE;
      }
      m_pDataStart = m_MemFile.GetReadPointer(0, ezMemoryMappedFile::OffsetBase::Start);

      if (ezArchiveUtils::ExtractZipTOC(m_MemFile, m_ArchiveTOC).Failed())
      {
        ezLog::Error("Failed to deserialize zip TOC");
        return EZ_FAILURE;
      }
    }
#  endif
    else
    {
      return EZ_FAILURE;
    }
  }

  // validate the entries
  {
    const ezUInt32 uiMaxPathString = m_ArchiveTOC.m_AllPathStrings.GetCount();
    const ezUInt64 uiValidSize = m_uiMemFileSize - uiMaxPathString;

    for (const auto& e : m_ArchiveTOC.m_Entries)
    {
      if (e.m_uiDataStartOffset + e.m_uiStoredDataSize > uiValidSize)
      {
        ezLog::Error("Archive is corrupt. Invalid entry data range.");
        return EZ_FAILURE;
      }

      if (e.m_uiUncompressedDataSize < e.m_uiStoredDataSize)
      {
        ezLog::Error("Archive is corrupt. Invalid compression info.");
        return EZ_FAILURE;
      }

      if (e.m_uiPathStringOffset >= uiMaxPathString)
      {
        ezLog::Error("Archive is corrupt. Invalid entry path-string offset.");
        return EZ_FAILURE;
      }
    }
  }

  return EZ_SUCCESS;
#else
  EZ_REPORT_FAILURE("Memory mapped files are unsupported on this platform.");
  return EZ_FAILURE;
#endif
}

const ezArchiveTOC& ezArchiveReader::GetArchiveTOC()
{
  return m_ArchiveTOC;
}

ezResult ezArchiveReader::ExtractAllFiles(const char* szTargetFolder) const
{
  EZ_LOG_BLOCK("ExtractAllFiles", szTargetFolder);

  const ezUInt32 numEntries = m_ArchiveTOC.m_Entries.GetCount();

  for (ezUInt32 e = 0; e < numEntries; ++e)
  {
    const char* szPath = reinterpret_cast<const char*>(&m_ArchiveTOC.m_AllPathStrings[m_ArchiveTOC.m_Entries[e].m_uiPathStringOffset]);

    if (!ExtractNextFileCallback(e + 1, numEntries, szPath))
      return EZ_FAILURE;

    EZ_SUCCEED_OR_RETURN(ExtractFile(e, szTargetFolder));
  }

  return EZ_SUCCESS;
}

void ezArchiveReader::ConfigureRawMemoryStreamReader(ezUInt32 uiEntryIdx, ezRawMemoryStreamReader& memReader) const
{
  ezArchiveUtils::ConfigureRawMemoryStreamReader(m_ArchiveTOC.m_Entries[uiEntryIdx], m_pDataStart, memReader);
}

ezUniquePtr<ezStreamReader> ezArchiveReader::CreateEntryReader(ezUInt32 uiEntryIdx) const
{
  return ezArchiveUtils::CreateEntryReader(m_ArchiveTOC.m_Entries[uiEntryIdx], m_pDataStart);
}

ezResult ezArchiveReader::ExtractFile(ezUInt32 uiEntryIdx, const char* szTargetFolder) const
{
  const char* szFilePath = m_ArchiveTOC.GetEntryPathString(uiEntryIdx);
  const ezUInt64 uiMaxSize = m_ArchiveTOC.m_Entries[uiEntryIdx].m_uiUncompressedDataSize;

  ezUniquePtr<ezStreamReader> pReader = CreateEntryReader(uiEntryIdx);

  ezStringBuilder sOutputFile = szTargetFolder;
  sOutputFile.AppendPath(szFilePath);

  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(sOutputFile));

  ezUInt8 uiTemp[1024 * 8];

  ezUInt64 uiRead = 0;
  ezUInt64 uiReadTotal = 0;
  while (true)
  {
    uiRead = pReader->ReadBytes(uiTemp, EZ_ARRAY_SIZE(uiTemp));

    if (uiRead == 0)
      break;

    EZ_SUCCEED_OR_RETURN(file.WriteBytes(uiTemp, uiRead));

    uiReadTotal += uiRead;

    if (!ExtractFileProgressCallback(uiReadTotal, uiMaxSize))
      return EZ_FAILURE;
  }

  EZ_ASSERT_DEV(uiReadTotal == uiMaxSize, "Failed to read entire file");

  return EZ_SUCCESS;
}

bool ezArchiveReader::ExtractNextFileCallback(ezUInt32 uiCurEntry, ezUInt32 uiMaxEntries, const char* szSourceFile) const
{
  return true;
}

bool ezArchiveReader::ExtractFileProgressCallback(ezUInt64 bytesWritten, ezUInt64 bytesTotal) const
{
  return true;
}


EZ_STATICLINK_FILE(Foundation, Foundation_IO_Archive_Implementation_ArchiveReader);
