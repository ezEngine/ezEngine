#include <FoundationPCH.h>

#include <Foundation/IO/Archive/ArchiveReader.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Types/Types.h>
#include <Foundation/IO/Archive/ArchiveUtils.h>

ezResult ezArchiveReader::OpenArchive(const char* szPath)
{
  EZ_LOG_BLOCK("OpenArchive", szPath);

  EZ_SUCCEED_OR_RETURN(m_MemFile.Open(szPath, ezMemoryMappedFile::Mode::ReadOnly));

  // validate the archive
  {
    ezRawMemoryStreamReader reader(m_MemFile.GetReadPointer(), m_MemFile.GetFileSize());

    EZ_SUCCEED_OR_RETURN(ezArchiveUtils::ReadHeader(reader, m_uiArchiveVersion));

    if (m_uiArchiveVersion != 1)
    {
      ezLog::Error("Unknown ezArchive version");
      return EZ_FAILURE;
    }
  }

  m_pDataStart = m_MemFile.GetReadPointer(16, ezMemoryMappedFile::OffsetBase::Start);

  if (ezArchiveUtils::ExtractTOC(m_MemFile, m_ArchiveTOC).Failed())
  {
    ezLog::Error("Failed to deserialize ezArchive TOC");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
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

  ezUInt8 uiTemp[1024 * 64];

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
