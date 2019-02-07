#include <FoundationPCH.h>

#include <Foundation/IO/DependencyFile.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

enum class ezDependencyFileVersion : ezUInt8
{
  Version0 = 0,
  Version1,
  Version2, ///< added 'sum' time

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
  m_uiSumTimeStampStored = 0;
  m_AssetTransformDependencies.Clear();
}

void ezDependencyFile::AddFileDependency(const char* szFile)
{
  if (ezStringUtils::IsNullOrEmpty(szFile))
    return;

  m_AssetTransformDependencies.PushBack(szFile);
}

void ezDependencyFile::StoreCurrentTimeStamp()
{
  EZ_LOG_BLOCK("ezDependencyFile::StoreCurrentTimeStamp");

  m_iMaxTimeStampStored = 0;
  m_uiSumTimeStampStored = 0;

#if EZ_DISABLED(EZ_SUPPORTS_FILE_STATS)
  ezLog::Warning("Trying to retrieve file time stamps on a platform that does not support it");
  return;
#endif

  for (const auto& sFile : m_AssetTransformDependencies)
  {
    ezTimestamp ts;
    if (RetrieveFileTimeStamp(sFile, ts).Failed())
      continue;

    const ezInt64 time = ts.GetInt64(ezSIUnitOfTime::Second);
    m_iMaxTimeStampStored = ezMath::Max<ezInt64>(m_iMaxTimeStampStored, time);
    m_uiSumTimeStampStored += (ezUInt64)time;
  }
}

bool ezDependencyFile::HasAnyFileChanged()
{
#if EZ_DISABLED(EZ_SUPPORTS_FILE_STATS)
  ezLog::Warning("Trying to retrieve file time stamps on a platform that does not support it");
  return true;
#endif

  ezUInt64 uiSumTs = 0;

  for (const auto& sFile : m_AssetTransformDependencies)
  {
    ezTimestamp ts;
    if (RetrieveFileTimeStamp(sFile, ts).Failed())
      continue;

    const ezInt64 time = ts.GetInt64(ezSIUnitOfTime::Second);

    if (time > m_iMaxTimeStampStored)
    {
      ezLog::Dev("Detected file change in '{0}' (TimeStamp {1} > MaxTimeStamp {2})", sFile, ts.GetInt64(ezSIUnitOfTime::Second), m_iMaxTimeStampStored);
      return true;
    }

    uiSumTs += (ezUInt64)time;
  }

  if (uiSumTs != m_uiSumTimeStampStored)
  {
    ezLog::Dev("Detected file change, but exact file is not known.");
    return true;
  }

  return false;
}

ezResult ezDependencyFile::WriteDependencyFile(ezStreamWriter& stream) const
{
  stream << (ezUInt8)ezDependencyFileVersion::Current;

  stream << m_iMaxTimeStampStored;
  stream << m_uiSumTimeStampStored;
  stream << m_AssetTransformDependencies.GetCount();

  for (const auto& sFile : m_AssetTransformDependencies)
    stream << sFile;

  return EZ_SUCCESS;
}

ezResult ezDependencyFile::ReadDependencyFile(ezStreamReader& stream)
{
  ezUInt8 uiVersion = (ezUInt8)ezDependencyFileVersion::Version0;
  stream >> uiVersion;

  if (uiVersion > (ezUInt8)ezDependencyFileVersion::Current)
  {
    ezLog::Error("Dependency file has incorrect file version ({0})", uiVersion);
    return EZ_FAILURE;
  }

  EZ_ASSERT_DEV(uiVersion <= (ezUInt8)ezDependencyFileVersion::Current, "Invalid file version {0}", uiVersion);

  stream >> m_iMaxTimeStampStored;

  if (uiVersion >= (ezUInt8)ezDependencyFileVersion::Version2)
  {
    stream >> m_uiSumTimeStampStored;
  }

  ezUInt32 count = 0;
  stream >> count;
  m_AssetTransformDependencies.SetCount(count);

  for (ezUInt32 i = 0; i < m_AssetTransformDependencies.GetCount(); ++i)
    stream >> m_AssetTransformDependencies[i];

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

    ezStringBuilder sAbsPath;
    if (ezFileSystem::ResolvePath(szFile, &sAbsPath, nullptr).Failed())
    {
      ezLog::Error("Could not resolve path for file '{0}'", szFile);
      return EZ_FAILURE;
    }

    ezFileStats stats;
    if (ezOSFile::GetFileStats(sAbsPath, stats).Failed())
    {
      ezLog::Error("Could not query the file stats for '{0}'", szFile);
      return EZ_FAILURE;
    }

    it.Value().m_FileTimestamp = stats.m_LastModificationTime;
  }

  out_Result = it.Value().m_FileTimestamp;

#else

  out_Result.SetInt64(0, ezSIUnitOfTime::Second);
  ezLog::Warning("Trying to retrieve a file time stamp on a platform that does not support it (file: '{0}')", szFile);

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



EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_DependencyFile);

