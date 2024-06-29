#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Stopwatch.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QDesktopServices>
#include <QDir>
#include <QIcon>
#include <QProcess>
#include <QScreen>
#include <QSettings>
#include <QUrl>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#  include <ShlObj_core.h>
#endif

EZ_IMPLEMENT_SINGLETON(ezQtUiServices);

ezEvent<const ezQtUiServices::Event&> ezQtUiServices::s_Events;
ezEvent<const ezQtUiServices::TickEvent&> ezQtUiServices::s_TickEvent;

ezMap<ezString, QIcon> ezQtUiServices::s_IconsCache;
ezMap<ezString, QImage> ezQtUiServices::s_ImagesCache;
ezMap<ezString, QPixmap> ezQtUiServices::s_PixmapsCache;
bool ezQtUiServices::s_bHeadless;
ezQtUiServices::TickEvent ezQtUiServices::s_LastTickEvent;

static ezQtUiServices* g_pInstance = nullptr;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, QtUiServices)

  ON_CORESYSTEMS_STARTUP
  {
    g_pInstance = EZ_DEFAULT_NEW(ezQtUiServices);
    ezQtUiServices::GetSingleton()->Init();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    EZ_DEFAULT_DELETE(g_pInstance);
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezQtUiServices::ezQtUiServices()
  : m_SingletonRegistrar(this)
{
  qRegisterMetaType<ezUuid>();
  m_pColorDlg = nullptr;
}


bool ezQtUiServices::IsHeadless()
{
  return s_bHeadless;
}


void ezQtUiServices::SetHeadless(bool bHeadless)
{
  s_bHeadless = true;
}

void ezQtUiServices::SaveState()
{
  QSettings Settings;
  Settings.beginGroup("EditorGUI");
  {
    Settings.setValue("ColorDlgGeom", m_ColorDlgGeometry);
  }
  Settings.endGroup();
}

ezTime g_Total = ezTime::MakeZero();

const QIcon& ezQtUiServices::GetCachedIconResource(ezStringView sIdentifier, ezColor svgTintColor)
{
  ezStringBuilder sFullIdentifier = sIdentifier;
  auto& map = s_IconsCache;

  const bool bNeedsColoring = svgTintColor != ezColor::MakeZero() && sIdentifier.EndsWith_NoCase(".svg");

  if (bNeedsColoring)
  {
    sFullIdentifier.AppendFormat("-{}", ezColorGammaUB(svgTintColor));
  }

  auto it = map.Find(sFullIdentifier);

  if (it.IsValid())
    return it.Value();

  if (bNeedsColoring)
  {
    ezStopwatch sw;

    // read the icon from the Qt virtual file system (QResource)
    QFile file(ezString(sIdentifier).GetData());
    if (!file.open(QIODeviceBase::OpenModeFlag::ReadOnly))
    {
      // if it doesn't exist, return an empty QIcon

      map[sFullIdentifier] = QIcon();
      return map[sFullIdentifier];
    }

    // get the entire SVG file content
    ezStringBuilder sContent = QString(file.readAll()).toUtf8().data();

    // replace the occurrence of the color white ("#FFFFFF") with the desired target color
    {
      const ezColorGammaUB color8 = svgTintColor;

      ezStringBuilder rep;
      rep.SetFormat("#{}{}{}", ezArgI((int)color8.r, 2, true, 16), ezArgI((int)color8.g, 2, true, 16), ezArgI((int)color8.b, 2, true, 16));

      sContent.ReplaceAll_NoCase("#ffffff", rep);

      rep.Append(";");
      sContent.ReplaceAll_NoCase("#fff;", rep);
      rep.Shrink(0, 1);

      rep.Prepend("\"");
      rep.Append("\"");
      sContent.ReplaceAll_NoCase("\"#fff\"", rep);
    }

    // hash the content AFTER the color replacement, so it includes the custom color change
    const ezUInt32 uiSrcHash = ezHashingUtils::xxHash32String(sContent);

    // file the path to the temp file, including the source hash
    const ezStringBuilder sTempFolder = ezOSFile::GetTempDataFolder("ezEditor/QIcons");
    ezStringBuilder sTempIconFile(sTempFolder, "/", sIdentifier.GetFileName());
    sTempIconFile.AppendFormat("-{}.svg", uiSrcHash);

    // only write to the file system, if the target file doesn't exist yet, this saves more than half the time
    if (!ezOSFile::ExistsFile(sTempIconFile))
    {
      // now write the new SVG file back to a dummy file
      // yes, this is as stupid as it sounds, we really write the file BACK TO THE FILESYSTEM, rather than doing this stuff in-memory
      // that's because I wasn't able to figure out whether we can somehow read a QIcon from a string rather than from file
      // it doesn't appear to be easy at least, since we can only give it a path, not a memory stream or anything like that
      {
        // necessary for Qt to be able to write to the folder
        ezOSFile::CreateDirectoryStructure(sTempFolder).AssertSuccess();

        QFile fileOut(sTempIconFile.GetData());
        fileOut.open(QIODeviceBase::OpenModeFlag::WriteOnly);
        fileOut.write(sContent.GetData(), sContent.GetElementCount());
        fileOut.flush();
        fileOut.close();
      }
    }

    QIcon icon(sTempIconFile.GetData());

    if (!icon.pixmap(QSize(16, 16)).isNull())
      map[sFullIdentifier] = icon;
    else
      map[sFullIdentifier] = QIcon();

    ezTime local = sw.GetRunningTotal();
    g_Total += local;

    // kept here for debug purposes, but don't waste time on logging
    // ezLog::Info("Icon load time: {}, total = {}", local, g_Total);
  }
  else
  {
    const QString sFile = ezString(sIdentifier).GetData();

    if (QFile::exists(sFile)) // prevent Qt from spamming warnings about non-existing files by checking this manually
    {
      QIcon icon(sFile);

      // Workaround for QIcon being stupid and treating failed to load icons as not-null.
      if (!icon.pixmap(QSize(16, 16)).isNull())
        map[sFullIdentifier] = icon;
      else
        map[sFullIdentifier] = QIcon();
    }
    else
      map[sFullIdentifier] = QIcon();
  }

  return map[sFullIdentifier];
}


const QImage& ezQtUiServices::GetCachedImageResource(const char* szIdentifier)
{
  const ezString sIdentifier = szIdentifier;
  auto& map = s_ImagesCache;

  auto it = map.Find(sIdentifier);

  if (it.IsValid())
    return it.Value();

  map[sIdentifier] = QImage(QString::fromUtf8(szIdentifier));

  return map[sIdentifier];
}

const QPixmap& ezQtUiServices::GetCachedPixmapResource(const char* szIdentifier)
{
  const ezString sIdentifier = szIdentifier;
  auto& map = s_PixmapsCache;

  auto it = map.Find(sIdentifier);

  if (it.IsValid())
    return it.Value();

  map[sIdentifier] = QPixmap(QString::fromUtf8(szIdentifier));

  return map[sIdentifier];
}

ezResult ezQtUiServices::AddToGitIgnore(const char* szGitIgnoreFile, const char* szPattern)
{
  ezStringBuilder ignoreFile;

  {
    ezFileReader file;
    if (file.Open(szGitIgnoreFile).Succeeded())
    {
      ignoreFile.ReadAll(file);
    }
  }

  ignoreFile.Trim("\n\r");

  const ezUInt32 len = ezStringUtils::GetStringElementCount(szPattern);

  // pattern already present ?
  if (const char* szFound = ignoreFile.FindSubString(szPattern))
  {
    if (szFound == ignoreFile.GetData() || // right at the start
        *(szFound - 1) == '\n')            // after a new line
    {
      const char end = *(szFound + len);

      if (end == '\0' || end == '\r' || end == '\n') // line does not continue with an extended pattern
      {
        return EZ_SUCCESS;
      }
    }
  }

  ignoreFile.AppendWithSeparator("\n", szPattern);
  ignoreFile.Append("\n\n");

  {
    ezFileWriter file;
    EZ_SUCCEED_OR_RETURN(file.Open(szGitIgnoreFile));

    EZ_SUCCEED_OR_RETURN(file.WriteBytes(ignoreFile.GetData(), ignoreFile.GetElementCount()));
  }

  return EZ_SUCCESS;
}

void ezQtUiServices::CheckForUpdates()
{
  Event e;
  e.m_Type = Event::Type::CheckForUpdates;
  s_Events.Broadcast(e);
}

void ezQtUiServices::Init()
{
  s_LastTickEvent.m_fRefreshRate = 60.0;
  if (QScreen* pScreen = QApplication::primaryScreen())
  {
    s_LastTickEvent.m_fRefreshRate = pScreen->refreshRate();
  }

  QTimer::singleShot((ezInt32)ezMath::Floor(1000.0 / s_LastTickEvent.m_fRefreshRate), this, SLOT(TickEventHandler()));
}

void ezQtUiServices::TickEventHandler()
{
  EZ_PROFILE_SCOPE("TickEvent");

  EZ_ASSERT_DEV(!m_bIsDrawingATM, "Implementation error");
  ezTime startTime = ezTime::Now();

  m_bIsDrawingATM = true;
  s_LastTickEvent.m_uiFrame++;
  s_LastTickEvent.m_Time = startTime;
  s_LastTickEvent.m_Type = TickEvent::Type::StartFrame;
  s_TickEvent.Broadcast(s_LastTickEvent);

  s_LastTickEvent.m_Type = TickEvent::Type::EndFrame;
  s_TickEvent.Broadcast(s_LastTickEvent);
  m_bIsDrawingATM = false;

  const ezTime endTime = ezTime::Now();
  ezTime lastFrameTime = endTime - startTime;

  ezTime delay = ezTime::MakeFromMilliseconds(1000.0 / s_LastTickEvent.m_fRefreshRate);
  delay -= lastFrameTime;
  delay = ezMath::Max(delay, ezTime::MakeZero());

  QTimer::singleShot((ezInt32)ezMath::Floor(delay.GetMilliseconds()), this, SLOT(TickEventHandler()));
}

void ezQtUiServices::LoadState()
{
  QSettings Settings;
  Settings.beginGroup("EditorGUI");
  {
    m_ColorDlgGeometry = Settings.value("ColorDlgGeom").toByteArray();
  }
  Settings.endGroup();
}

void ezQtUiServices::ShowAllDocumentsTemporaryStatusBarMessage(const ezFormatString& msg, ezTime timeOut)
{
  ezStringBuilder tmp;

  Event e;
  e.m_Type = Event::ShowDocumentTemporaryStatusBarText;
  e.m_sText = msg.GetText(tmp);
  e.m_Time = timeOut;

  s_Events.Broadcast(e, 1);
}

void ezQtUiServices::ShowAllDocumentsPermanentStatusBarMessage(const ezFormatString& msg, Event::TextType type)
{
  ezStringBuilder tmp;

  Event e;
  e.m_Type = Event::ShowDocumentPermanentStatusBarText;
  e.m_sText = msg.GetText(tmp);
  e.m_TextType = type;

  s_Events.Broadcast(e, 1);
}

void ezQtUiServices::ShowGlobalStatusBarMessage(const ezFormatString& msg)
{
  ezStringBuilder tmp;

  Event e;
  e.m_Type = Event::ShowGlobalStatusBarText;
  e.m_sText = msg.GetText(tmp);
  e.m_Time = ezTime::MakeFromSeconds(0);

  s_Events.Broadcast(e);
}


bool ezQtUiServices::OpenFileInDefaultProgram(const char* szPath)
{
  return QDesktopServices::openUrl(QUrl::fromLocalFile(szPath));
}

void ezQtUiServices::OpenInExplorer(const char* szPath, bool bIsFile)
{
  QStringList args;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  if (bIsFile)
    args << "/select,";

  args << QDir::toNativeSeparators(szPath);

  QProcess::startDetached("explorer", args);
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
  ezStringBuilder parentDir;

  if (bIsFile)
  {
    parentDir = szPath;
    parentDir = parentDir.GetFileDirectory();
    szPath = parentDir.GetData();
  }
  args << QDir::toNativeSeparators(szPath);

  QProcess::startDetached("xdg-open", args);
#else
  EZ_ASSERT_NOT_IMPLEMENTED
#endif
}

void ezQtUiServices::OpenWith(const char* szPath)
{
  ezStringBuilder sPath = szPath;
  sPath.MakeCleanPath();
  sPath.MakePathSeparatorsNative();

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  ezStringWChar wpath(sPath);
  OPENASINFO oi;
  oi.pcszFile = wpath.GetData();
  oi.pcszClass = NULL;
  oi.oaifInFlags = OAIF_EXEC;
  SHOpenWithDialog(NULL, &oi);
#else
  EZ_ASSERT_NOT_IMPLEMENTED
#endif
}

ezStatus ezQtUiServices::OpenInVsCode(const QStringList& arguments)
{
  QString sVsCodeExe;
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  sVsCodeExe =
    QStandardPaths::locate(QStandardPaths::GenericDataLocation, "Programs/Microsoft VS Code/Code.exe", QStandardPaths::LocateOption::LocateFile);

  if (!QFile().exists(sVsCodeExe))
  {
    QSettings settings("\\HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\Applications\\Code.exe\\shell\\open\\command", QSettings::NativeFormat);
    QString sVsCodeExeKey = settings.value(".", "").value<QString>();

    if (sVsCodeExeKey.length() > 5)
    {
      // Remove shell parameter and normalize QT Compatible path, QFile expects the file separator to be '/' regardless of operating system
      sVsCodeExe = sVsCodeExeKey.left(sVsCodeExeKey.length() - 5).replace("\\", "/").replace("\"", "");
    }
  }
#endif

  if (sVsCodeExe.isEmpty() || !QFile().exists(sVsCodeExe))
  {
    // Try code executable in PATH
    if (QProcess::execute("code", {"--version"}) == 0)
    {
      sVsCodeExe = "code";
    }
    else
    {
      return ezStatus("Installation of Visual Studio Code could not be located.\n"
                      "Please visit 'https://code.visualstudio.com/download' to download the 'User Installer' of Visual Studio Code.");
    }
  }

  QProcess proc;
  if (proc.startDetached(sVsCodeExe, arguments) == false)
  {
    return ezStatus("Failed to launch Visual Studio Code.");
  }

  return ezStatus(EZ_SUCCESS);
}
