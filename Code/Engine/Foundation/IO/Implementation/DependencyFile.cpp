#include <Foundation/FoundationPCH.h>

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

ezMutex ezDependencyFile::s_FileTimestampsLock;
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

void ezDependencyFile::AddFileDependency(ezStringView sFile)
{
  if (sFile.IsEmpty())
    return;

  m_AssetTransformDependencies.PushBack(sFile);
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

bool ezDependencyFile::HasAnyFileChanged() const
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
      ezLog::Dev("Detected file change in '{0}' (TimeStamp {1} > MaxTimeStamp {2})", ezArgSensitive(sFile, "File"),
        ts.GetInt64(ezSIUnitOfTime::Second), m_iMaxTimeStampStored);
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

ezResult ezDependencyFile::WriteDependencyFile(ezStreamWriter& inout_stream) const
{
  inout_stream << (ezUInt8)ezDependencyFileVersion::Current;

  inout_stream << m_iMaxTimeStampStored;
  inout_stream << m_uiSumTimeStampStored;
  inout_stream << m_AssetTransformDependencies.GetCount();

  for (const auto& sFile : m_AssetTransformDependencies)
    inout_stream << sFile;

  return EZ_SUCCESS;
}

ezResult ezDependencyFile::ReadDependencyFile(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = (ezUInt8)ezDependencyFileVersion::Version0;
  inout_stream >> uiVersion;

  if (uiVersion > (ezUInt8)ezDependencyFileVersion::Current)
  {
    ezLog::Error("Dependency file has incorrect file version ({0})", uiVersion);
    return EZ_FAILURE;
  }

  EZ_ASSERT_DEV(uiVersion <= (ezUInt8)ezDependencyFileVersion::Current, "Invalid file version {0}", uiVersion);

  inout_stream >> m_iMaxTimeStampStored;

  if (uiVersion >= (ezUInt8)ezDependencyFileVersion::Version2)
  {
    inout_stream >> m_uiSumTimeStampStored;
  }

  ezUInt32 count = 0;
  inout_stream >> count;
  m_AssetTransformDependencies.SetCount(count);

  for (ezUInt32 i = 0; i < m_AssetTransformDependencies.GetCount(); ++i)
    inout_stream >> m_AssetTransformDependencies[i];

  return EZ_SUCCESS;
}

ezResult ezDependencyFile::RetrieveFileTimeStamp(ezStringView sFile, ezTimestamp& out_Result)
{
#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
  EZ_LOCK(s_FileTimestampsLock);
  bool bExisted = false;
  auto it = s_FileTimestamps.FindOrAdd(sFile, &bExisted);

  if (!bExisted || it.Value().m_LastCheck + ezTime::MakeFromSeconds(2.0) < ezTime::Now())
  {
    it.Value().m_LastCheck = ezTime::Now();

    ezFileStats stats;
    if (ezFileSystem::GetFileStats(sFile, stats).Failed())
    {
      ezLog::Error("Could not query the file stats for '{0}'", ezArgSensitive(sFile, "File"));
      return EZ_FAILURE;
    }

    it.Value().m_FileTimestamp = stats.m_LastModificationTime;
  }

  out_Result = it.Value().m_FileTimestamp;

#else

  out_Result = ezTimestamp::MakeFromInt(0, ezSIUnitOfTime::Second);
  ezLog::Warning("Trying to retrieve a file time stamp on a platform that does not support it (file: '{0}')", ezArgSensitive(sFile, "File"));

#endif

  return out_Result.IsValid() ? EZ_SUCCESS : EZ_FAILURE;
}

ezResult ezDependencyFile::WriteDependencyFile(ezStringView sFile) const
{
  EZ_LOG_BLOCK("ezDependencyFile::WriteDependencyFile", sFile);

  ezFileWriter file;
  if (file.Open(sFile).Failed())
    return EZ_FAILURE;

  return WriteDependencyFile(file);
}

ezResult ezDependencyFile::ReadDependencyFile(ezStringView sFile)
{
  EZ_LOG_BLOCK("ezDependencyFile::ReadDependencyFile", sFile);

  ezFileReader file;
  if (file.Open(sFile).Failed())
    return EZ_FAILURE;

  return ReadDependencyFile(file);
}
