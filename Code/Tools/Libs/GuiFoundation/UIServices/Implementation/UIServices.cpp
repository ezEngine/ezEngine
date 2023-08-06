#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QDesktopServices>
#include <QDir>
#include <QIcon>
#include <QProcess>
#include <QScreen>
#include <QSettings>
#include <QUrl>

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

ezMutex m;

const QIcon& ezQtUiServices::GetCachedIconResource(const char* szIdentifier, ezColor color)
{
  ezStringBuilder sIdentifier = szIdentifier;
  auto& map = s_IconsCache;

  if (color != ezColor::MakeZero())
  {
    sIdentifier.AppendFormat("-{}", ezColorGammaUB(color));
  }

  auto it = map.Find(sIdentifier);

  if (it.IsValid())
    return it.Value();

  ezStringBuilder filename = sIdentifier.GetFileName();

  if (color != ezColor::MakeZero())
  {
    EZ_LOCK(m);

    const ezColorGammaUB color8 = color;

    ezOSFile::CreateDirectoryStructure(ezOSFile::GetTempDataFolder("QIcons")).AssertSuccess();
    const ezStringBuilder name(ezOSFile::GetTempDataFolder("QIcons"), "/", filename, ".svg");

    QFile file(szIdentifier);
    file.open(QIODeviceBase::OpenModeFlag::ReadOnly);
    QByteArray content = file.readAll();
    file.close();

    QString sContent = content;

    const QChar c0 = '0';

    sContent = sContent.replace("#ffffff", QString("#%1%2%3").arg((int)color8.r, 2, 16, c0).arg((int)color8.g, 2, 16, c0).arg((int)color8.b, 2, 16, c0), Qt::CaseInsensitive);

    {
      ezStringBuilder tmp = sContent.toUtf8().data();

      QFile fileOut(name.GetData());
      fileOut.open(QIODeviceBase::OpenModeFlag::WriteOnly);
      fileOut.write(tmp.GetData(), tmp.GetElementCount());
      fileOut.flush();
      fileOut.close();
    }

    QIcon icon(name.GetData());

    if (!icon.pixmap(QSize(16, 16)).isNull())
      map[sIdentifier] = icon;
    else
      map[sIdentifier] = QIcon();
  }
  else
  {
    QIcon icon(szIdentifier);

    // Workaround for QIcon being stupid and treating failed to load icons as not-null.
    if (!icon.pixmap(QSize(16, 16)).isNull())
      map[sIdentifier] = icon;
    else
      map[sIdentifier] = QIcon();
  }

  return map[sIdentifier];
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

ezStatus ezQtUiServices::OpenInVsCode(const QStringList& arguments)
{
  QString sVsCodeExe =
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

  if (!QFile().exists(sVsCodeExe))
  {
    return ezStatus("Installation of Visual Studio Code could not be located.\n"
                    "Please visit 'https://code.visualstudio.com/download' to download the 'User Installer' of Visual Studio Code.");
  }

  QProcess proc;
  if (proc.startDetached(sVsCodeExe, arguments) == false)
  {
    return ezStatus("Failed to launch Visual Studio Code.");
  }

  return ezStatus(EZ_SUCCESS);
}
