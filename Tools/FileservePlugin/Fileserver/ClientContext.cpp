#include <PCH.h>
#include <FileservePlugin/Fileserver/ClientContext.h>
#include <Foundation/IO/FileSystem/FileReader.h>

ezFileserveFileState ezFileserveClientContext::GetFileStatus(ezUInt16 uiDataDirID, const char* szRequestedFile, FileStatus& inout_Status, ezDynamicArray<ezUInt8>& out_FileContent) const
{
  const auto& dd = m_MountedDataDirs[uiDataDirID];

  ezStringBuilder sAbsPath;
  sAbsPath = dd.m_sPathOnServer;
  sAbsPath.AppendPath(szRequestedFile);

  ezFileStats stat;
  if (ezOSFile::GetFileStats(sAbsPath, stat).Failed())
  {
    // the client doesn't have the file either
    // this is an optimization to prevent redundant file deletions on the client
    if (inout_Status.m_iTimestamp == 0 && inout_Status.m_uiHash == 0)
      return ezFileserveFileState::SameTimestamp;

    return ezFileserveFileState::NonExistant;
  }

  inout_Status.m_uiFileSize = stat.m_uiFileSize;

  const ezInt64 iNewTimestamp = stat.m_LastModificationTime.GetInt64(ezSIUnitOfTime::Microsecond);

  if (inout_Status.m_iTimestamp == iNewTimestamp)
    return ezFileserveFileState::SameTimestamp;

  inout_Status.m_iTimestamp = iNewTimestamp;

  // read the entire file
  {
    ezFileReader file;
    if (file.Open(sAbsPath).Failed())
      return ezFileserveFileState::NonExistant;

    ezUInt64 uiNewHash = 0;
    out_FileContent.SetCountUninitialized((ezUInt32)inout_Status.m_uiFileSize);

    if (!out_FileContent.IsEmpty())
    {
      file.ReadBytes(out_FileContent.GetData(), out_FileContent.GetCount());
      uiNewHash = ezHashing::MurmurHash64(out_FileContent.GetData(), (size_t)out_FileContent.GetCount());
    }

    if (inout_Status.m_uiHash == uiNewHash)
      return ezFileserveFileState::SameHash;

    inout_Status.m_uiHash = uiNewHash;
  }

  return ezFileserveFileState::Different;
}

