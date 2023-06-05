#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/CheckVersion.moc.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <QNetworkReply>
#include <QProcess>

static ezString GetVersionFilePath()
{
  ezStringBuilder sTemp = ezOSFile::GetTempDataFolder();
  sTemp.AppendPath("ezEditor/version-page.htm");
  return sTemp;
}

PageDownloader::PageDownloader(const QString& sUrl)
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  QStringList args;

  args << "-Command";
  args << QString("(Invoke-webrequest -URI \"%1\").Content > \"%2\"").arg(sUrl).arg(GetVersionFilePath().GetData());

  m_pProcess = EZ_DEFAULT_NEW(QProcess);
  connect(m_pProcess.Borrow(), &QProcess::finished, this, &PageDownloader::DownloadDone);
  m_pProcess->start("C:\\Windows\\System32\\WindowsPowershell\\v1.0\\powershell.exe", args);
#else
  EZ_ASSERT_NOT_IMPLEMENTED;
#endif
}

void PageDownloader::DownloadDone(int exitCode, QProcess::ExitStatus exitStatus)
{
  m_pProcess = nullptr;

  ezOSFile file;
  if (file.Open(GetVersionFilePath(), ezFileOpenMode::Read).Failed())
    return;

  ezDataBuffer content;
  file.ReadAll(content);

  content.PushBack('\0');
  content.PushBack('\0');

  const ezUInt16* pStart = (ezUInt16*)content.GetData();
  if (ezUnicodeUtils::SkipUtf16BomLE(pStart))
  {
    m_sDownloadedPage = ezStringWChar(pStart);
  }
  else
  {
    const char* szUtf8 = (const char*)content.GetData();
    m_sDownloadedPage = ezStringWChar(szUtf8);
  }

  ezOSFile::DeleteFile(GetVersionFilePath()).IgnoreResult();

  Q_EMIT FinishedDownload();
}

ezQtVersionChecker::ezQtVersionChecker()
{
  m_sKnownLatestVersion = GetOwnVersion();
  m_sConfigFile = ":appdata/VersionCheck.ddl";
}

void ezQtVersionChecker::Initialize()
{
  m_bRequireOnlineCheck = true;

  ezFileStats fs;
  if (ezFileSystem::GetFileStats(m_sConfigFile, fs).Failed())
    return;

  ezFileReader file;
  if (file.Open(m_sConfigFile).Failed())
    return;

  ezOpenDdlReader ddl;
  if (ddl.ParseDocument(file).Failed())
    return;

  auto pRoot = ddl.GetRootElement();
  if (pRoot == nullptr)
    return;

  auto pLatest = pRoot->FindChild("KnownLatest");
  if (pLatest == nullptr || !pLatest->HasPrimitives(ezOpenDdlPrimitiveType::String))
    return;

  m_sKnownLatestVersion = pLatest->GetPrimitivesString()[0];

  const ezTimestamp nextCheck = fs.m_LastModificationTime + ezTime::Hours(24);

  if (nextCheck.Compare(ezTimestamp::CurrentTimestamp(), ezTimestamp::CompareMode::Newer))
  {
    // everything fine, we already checked within the last 24 hours

    m_bRequireOnlineCheck = false;
    return;
  }
}

ezResult ezQtVersionChecker::StoreKnownVersion()
{
  ezFileWriter file;
  if (file.Open(m_sConfigFile).Failed())
    return EZ_FAILURE;

  ezOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);
  ezOpenDdlUtils::StoreString(ddl, m_sKnownLatestVersion, "KnownLatest");

  return EZ_SUCCESS;
}

bool ezQtVersionChecker::Check(bool bForce)
{
#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  EZ_ASSERT_DEV(!bForce, "The version check is not yet implemented on this platform.");
  return false;
#endif

  if (bForce)
  {
    // to trigger a 'new release available' signal
    m_sKnownLatestVersion = GetOwnVersion();
    m_bForceCheck = true;
  }

  if (m_bCheckInProgresss)
    return false;

  if (!bForce && !m_bRequireOnlineCheck)
  {
    Q_EMIT VersionCheckCompleted(false, false);
    return false;
  }

  m_bCheckInProgresss = true;

  m_pVersionPage = EZ_DEFAULT_NEW(PageDownloader, "https://ezengine.net/pages/getting-started/binaries.html");

  connect(m_pVersionPage.Borrow(), &PageDownloader::FinishedDownload, this, &ezQtVersionChecker::PageDownloaded);

  return true;
}

const char* ezQtVersionChecker::GetOwnVersion() const
{
  return BUILDSYSTEM_SDKVERSION_MAJOR "." BUILDSYSTEM_SDKVERSION_MINOR "." BUILDSYSTEM_SDKVERSION_PATCH;
}

const char* ezQtVersionChecker::GetKnownLatestVersion() const
{
  return m_sKnownLatestVersion;
}

bool ezQtVersionChecker::IsLatestNewer() const
{
  const char* szParsePos;
  ezUInt32 own[3] = {0, 0, 0};
  ezUInt32 cur[3] = {0, 0, 0};

  szParsePos = GetOwnVersion();
  for (ezUInt32 i : {0, 1, 2})
  {
    if (ezConversionUtils::StringToUInt(szParsePos, own[i], &szParsePos).Failed())
      break;

    if (*szParsePos == '.')
      ++szParsePos;
    else
      break;
  }

  szParsePos = GetKnownLatestVersion();
  for (ezUInt32 i : {0, 1, 2})
  {
    if (ezConversionUtils::StringToUInt(szParsePos, cur[i], &szParsePos).Failed())
      break;

    if (*szParsePos == '.')
      ++szParsePos;
    else
      break;
  }

  // 'major'
  if (own[0] > cur[0])
    return false;
  if (own[0] < cur[0])
    return true;

  // 'minor'
  if (own[1] > cur[1])
    return false;
  if (own[1] < cur[1])
    return true;

  // 'patch'
  return own[2] < cur[2];
}

void ezQtVersionChecker::PageDownloaded()
{
  m_bCheckInProgresss = false;
  ezStringBuilder sPage = m_pVersionPage->GetDownloadedData();

  m_pVersionPage = nullptr;

  if (sPage.IsEmpty())
  {
    ezLog::Warning("Could not download release notes page.");
    return;
  }

  const char* szVersionStartTag = "<!--<VERSION>-->";
  const char* szVersionEndTag = "<!--</VERSION>-->";

  const char* pVersionStart = sPage.FindSubString(szVersionStartTag);
  const char* pVersionEnd = sPage.FindSubString(szVersionEndTag, pVersionStart);

  if (pVersionStart == nullptr || pVersionEnd == nullptr)
  {
    ezLog::Warning("Version check failed.");
    return;
  }

  ezStringBuilder sVersion;
  sVersion.SetSubString_FromTo(pVersionStart + ezStringUtils::GetStringElementCount(szVersionStartTag), pVersionEnd);

  const bool bNewRelease = m_sKnownLatestVersion != sVersion;

  // make sure to modify the file, even if the version is the same
  m_sKnownLatestVersion = sVersion;
  if (StoreKnownVersion().Failed())
  {
    ezLog::Warning("Could not store the last known version file.");
  }

  Q_EMIT VersionCheckCompleted(bNewRelease, m_bForceCheck);
}
