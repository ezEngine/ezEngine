#include <Foundation/PCH.h>
#include <Foundation/IO/DependencyFile.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/IO/OSFile.h>

enum class ezDependencyFileVersion : ezUInt8
{
  Version0 = 0,
  Version1,

  ENUM_COUNT,
  Current = ENUM_COUNT - 1,
};

ezMap<ezString, ezDependencyFile::FileCheckCache> ezDependencyFile::s_FileTimestamps;

ezDependencyFile::ezDependencyFile()
{
  Clear();
}

void ezDependencyFile::Clear()
{
  m_iMaxTimeStampStored = 0;
  m_FileDependencies.Clear();
}

void ezDependencyFile::AddFileDependency(const char* szFile)
{
  if (ezStringUtils::IsNullOrEmpty(szFile))
    return;

  m_FileDependencies.PushBack(szFile);
}

void ezDependencyFile::StoreCurrentTimeStamp()
{
  EZ_LOG_BLOCK("ezDependencyFile::StoreCurrentTimeStamp");

  m_iMaxTimeStampStored = 0;

#if EZ_DISABLED(EZ_SUPPORTS_FILE_STATS)
  ezLog::Warning("Trying to retrieve file time stamps on a platform that does not support it");
  return;
#endif

  for (const auto& sFile : m_FileDependencies)
  {
    ezTimestamp ts;
    if (RetrieveFileTimeStamp(sFile, ts).Failed())
      continue;

    m_iMaxTimeStampStored = ezMath::Max<ezInt64>(m_iMaxTimeStampStored, ts.GetInt64(ezSIUnitOfTime::Second));
  }
}

bool ezDependencyFile::HasAnyFileChanged()
{
#if EZ_DISABLED(EZ_SUPPORTS_FILE_STATS)
  ezLog::Warning("Trying to retrieve file time stamps on a platform that does not support it");
  return true;
#endif

  for (const auto& sFile : m_FileDependencies)
  {
    ezTimestamp ts;
    if (RetrieveFileTimeStamp(sFile, ts).Failed())
      continue;

    if (ts.GetInt64(ezSIUnitOfTime::Second) > m_iMaxTimeStampStored)
    {
      ezLog::Dev("Detected file change in '%s' (TimeStamp %lli > MaxTimeStamp %lli)", sFile.GetData(), ts.GetInt64(ezSIUnitOfTime::Second), m_iMaxTimeStampStored);
      return true;
    }
  }

  return false;
}

ezResult ezDependencyFile::WriteDependencyFile(ezStreamWriterBase& stream) const
{
  stream << (ezUInt8) ezDependencyFileVersion::Current;
  
  stream << m_iMaxTimeStampStored;
  stream << m_FileDependencies.GetCount();

  for (const auto& sFile : m_FileDependencies)
    stream << sFile;

  return EZ_SUCCESS;
}

ezResult ezDependencyFile::ReadDependencyFile(ezStreamReaderBase& stream)
{
  ezUInt8 uiVersion = (ezUInt8) ezDependencyFileVersion::Version0;
  stream >> uiVersion;

  if (uiVersion != (ezUInt8) ezDependencyFileVersion::Version1)
  {
    ezLog::Error("Dependency file has incorrect file version (%u)", uiVersion);
    return EZ_FAILURE;
  }
  
  EZ_ASSERT_DEV(uiVersion <= (ezUInt8) ezDependencyFileVersion::Current, "Invalid file version %u", uiVersion);
  
  stream >> m_iMaxTimeStampStored;

  ezUInt32 count = 0;
  stream >> count;
   m_FileDependencies.SetCount(count);

  for (ezUInt32 i = 0; i < m_FileDependencies.GetCount(); ++i)
    stream >> m_FileDependencies[i];

  return EZ_SUCCESS;
}

ezResult ezDependencyFile::RetrieveFileTimeStamp(const char* szFile, ezTimestamp& out_Result)
{
#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)

  bool bExisted = false;
  auto it = s_FileTimestamps.FindOrAdd(szFile, &bExisted);

  if (!bExisted || it.Value().m_LastCheck + ezTime::Seconds(2.0) < ezTime::Now())
  {
    it.Value().m_LastCheck = ezTime::Now();

    ezString sAbsPath;
    if (ezFileSystem::ResolvePath(szFile, false, &sAbsPath, nullptr).Failed())
    {
      ezLog::Error("Could not resolve path for file '%s'", szFile);
      return EZ_FAILURE;
    }

    ezFileStats stats;
    if (ezOSFile::GetFileStats(sAbsPath.GetData(), stats).Failed())
    {
      ezLog::Error("Could not query the file stats for '%s'", szFile);
      return EZ_FAILURE;
    }

    it.Value().m_FileTimestamp = stats.m_LastModificationTime;
  }

  out_Result = it.Value().m_FileTimestamp;

#else

  out_Result.SetInt64(0, ezSIUnitOfTime::Second);
  ezLog::Warning("Trying to retrieve a file time stamp on a platform that does not support it (file: '%s')", szFile);

#endif

  return out_Result.IsValid() ? EZ_SUCCESS : EZ_FAILURE;
}

ezResult ezDependencyFile::WriteDependencyFile(const char* szFile) const
{
  EZ_LOG_BLOCK("ezDependencyFile::WriteDependencyFile", szFile);

  ezFileWriter file;
  if (file.Open(szFile).Failed())
    return EZ_FAILURE;

  return WriteDependencyFile(file);
}

ezResult ezDependencyFile::ReadDependencyFile(const char* szFile)
{
  EZ_LOG_BLOCK("ezDependencyFile::ReadDependencyFile", szFile);

  ezFileReader file;
  if (file.Open(szFile).Failed())
    return EZ_FAILURE;

  return ReadDependencyFile(file);
}