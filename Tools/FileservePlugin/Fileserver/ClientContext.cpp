#include <PCH.h>
#include <FileservePlugin/Fileserver/ClientContext.h>

ezResult ezFileserveClientContext::FindFileInDataDirs(ezUInt16 uiDataDirID, const char* szRequestedFile, ezStringBuilder& out_sRelPath, ezStringBuilder& out_sAbsPath, const ezFileserveClientContext::DataDir** ppDataDir) const
{
  EZ_ASSERT_DEV(uiDataDirID < m_MountedDataDirs.GetCount(), "Invalid data dir ID");

  const auto& dd = m_MountedDataDirs[uiDataDirID];

  const char* szSubPathToUse = szRequestedFile;

  if (szRequestedFile[0] == ':') // a rooted path
  {
    // dd.m_sRootName already ends with a / to prevent incorrect matches
    if (!ezStringUtils::StartsWith(szRequestedFile, dd.m_sRootName))
    {
      ezLog::Warning("Path is rooted but does not match data dir: {0}", szRequestedFile);
      return EZ_FAILURE;
    }

    szSubPathToUse = szRequestedFile + dd.m_sRootName.GetElementCount();
    ezLog::Warning("Found data dir for rooted path: '{0}' -> '{1}'", szRequestedFile, szSubPathToUse);
  }
  else if (ezStringUtils::StartsWith(szRequestedFile, dd.m_sPathOnClient))
  {
    szSubPathToUse = szRequestedFile + dd.m_sPathOnClient.GetElementCount();

    ezLog::Warning("Found file with client prefix: '{0}' | '{1}'", dd.m_sPathOnClient, szSubPathToUse);
  }

  out_sAbsPath = dd.m_sPathOnServer;
  out_sAbsPath.AppendPath(szSubPathToUse);

  if (ezOSFile::ExistsFile(out_sAbsPath))
  {
    out_sRelPath = szSubPathToUse;

    if (ppDataDir)
    {
      *ppDataDir = &dd;
    }

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

