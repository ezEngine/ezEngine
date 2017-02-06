#include <PCH.h>
#include <FileservePlugin/Fileserver/ClientContext.h>

ezResult ezFileserveClientContext::FindFileInDataDirs(const char* szRequestedFile, ezStringBuilder& out_sRelPath, ezStringBuilder& out_sAbsPath, const ezFileserveClientContext::DataDir** ppDataDir) const
{
  for (ezUInt32 i = m_MountedDataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_MountedDataDirs[i - 1];

    const char* szSubPathToUse = szRequestedFile;

    if (szRequestedFile[0] == ':') // a rooted path
    {
      //ezLog::Warning("Path is rooted: {0}", szRequestedFile);

      // skip all data dirs that do not have the same root name
      // dd.m_sRootName already ends with a / to prevent incorrect matches
      if (!ezStringUtils::StartsWith(szRequestedFile, dd.m_sRootName))
        continue;

      szSubPathToUse = szRequestedFile + dd.m_sRootName.GetElementCount();
      //ezLog::Warning("Found data dir for rooted path: '{0}' -> '{1}'", szRequestedFile, szSubPathToUse);
    }
    else if (ezStringUtils::StartsWith(szRequestedFile, dd.m_sPathOnClient))
    {
      szSubPathToUse = szRequestedFile + dd.m_sPathOnClient.GetElementCount();

      //ezLog::Warning("Found file with client prefix: '{0}' | '{1}'", dd.m_sPathOnClient, szSubPathToUse);
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
  }

  return EZ_FAILURE;
}

