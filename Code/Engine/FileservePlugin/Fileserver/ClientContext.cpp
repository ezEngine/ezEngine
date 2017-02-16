#include <FileservePlugin/PCH.h>
#include <FileservePlugin/Fileserver/ClientContext.h>
#include <Foundation/IO/FileSystem/FileReader.h>

ezFileserveFileState ezFileserveClientContext::GetFileStatus(ezUInt16& inout_uiDataDirID, const char* szRequestedFile, FileStatus& inout_Status, ezDynamicArray<ezUInt8>& out_FileContent, bool bForceThisDataDir) const
{
  for (ezUInt32 i = m_MountedDataDirs.GetCount(); i > 0; --i)
  {
    const ezUInt16 uiDataDirID = i - 1;

    // when !bForceThisDataDir search for the best match
    // otherwise lookup this exact data dir
    if (bForceThisDataDir && uiDataDirID != inout_uiDataDirID)
      continue;

    const auto& dd = m_MountedDataDirs[uiDataDirID];

    if (!dd.m_bMounted)
      continue;

    ezStringBuilder sAbsPath;
    sAbsPath = dd.m_sPathOnServer;
    sAbsPath.AppendPath(szRequestedFile);

    ezFileStats stat;
    if (ezOSFile::GetFileStats(sAbsPath, stat).Failed())
      continue;

    inout_Status.m_uiFileSize = stat.m_uiFileSize;

    const ezInt64 iNewTimestamp = stat.m_LastModificationTime.GetInt64(ezSIUnitOfTime::Microsecond);

    if (inout_Status.m_iTimestamp == iNewTimestamp && inout_uiDataDirID == uiDataDirID)
      return ezFileserveFileState::SameTimestamp;

    inout_Status.m_iTimestamp = iNewTimestamp;

    // read the entire file
    {
      ezFileReader file;
      if (file.Open(sAbsPath).Failed())
        continue;

      ezUInt64 uiNewHash = 0;
      out_FileContent.SetCountUninitialized((ezUInt32)inout_Status.m_uiFileSize);

      if (!out_FileContent.IsEmpty())
      {
        file.ReadBytes(out_FileContent.GetData(), out_FileContent.GetCount());
        uiNewHash = ezHashing::MurmurHash64(out_FileContent.GetData(), (size_t)out_FileContent.GetCount());

        // if the file is empty, the hash will be zero, which could lead to an incorrect assumption that the hash is the same
        // instead always transfer the empty file, so that it properly exists on the client
        if (inout_Status.m_uiHash == uiNewHash && inout_uiDataDirID == uiDataDirID)
          return ezFileserveFileState::SameHash;
      }

      inout_Status.m_uiHash = uiNewHash;
    }

    inout_uiDataDirID = uiDataDirID;
    return ezFileserveFileState::Different;
  }

  inout_Status.m_iTimestamp = 0;
  inout_Status.m_uiFileSize = 0;
  inout_Status.m_uiHash = 0;

  // the client doesn't have the file either
  // this is an optimization to prevent redundant file deletions on the client
  if (inout_Status.m_iTimestamp == 0 && inout_Status.m_uiHash == 0)
    return ezFileserveFileState::NonExistantEither;

  return ezFileserveFileState::NonExistant;
}

