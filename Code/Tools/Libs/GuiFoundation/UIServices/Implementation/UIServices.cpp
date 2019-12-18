#include <GuiFoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QDesktopServices>
#include <QDir>
#include <QIcon>
#include <QProcess>
#include <QSettings>
#include <QUrl>

EZ_IMPLEMENT_SINGLETON(ezQtUiServices);

ezEvent<const ezQtUiServices::Event&> ezQtUiServices::s_Events;
ezMap<ezString, QIcon> ezQtUiServices::s_IconsCache;
ezMap<ezString, QImage> ezQtUiServices::s_ImagesCache;
ezMap<ezString, QPixmap> ezQtUiServices::s_PixmapsCache;
bool ezQtUiServices::s_bHeadless;


static ezQtUiServices g_instance;

ezQtUiServices::ezQtUiServices()
  : m_SingletonRegistrar(this)
{
  int id = qRegisterMetaType<ezUuid>();
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


const QIcon& ezQtUiServices::GetCachedIconResource(const char* szIdentifier)
{
  const ezString sIdentifier = szIdentifier;
  auto& map = s_IconsCache;

  auto it = map.Find(sIdentifier);

  if (it.IsValid())
    return it.Value();

  QIcon icon(QString::fromUtf8(szIdentifier));

  // Workaround for QIcon being stupid and treating failed to load icons as not-null.
  if (!icon.pixmap(QSize(16, 16)).isNull())
    map[sIdentifier] = icon;
  else
    map[sIdentifier] = QIcon();

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

  const ezStringBuilder sPattern(szPattern, "\n");

  // pattern already present ?
  if (ignoreFile.FindSubString(sPattern))
    return EZ_SUCCESS;

  ignoreFile.AppendWithSeparator("\n", sPattern);

  {
    ezFileWriter file;
    EZ_SUCCEED_OR_RETURN(file.Open(szGitIgnoreFile));

    EZ_SUCCEED_OR_RETURN(file.WriteBytes(ignoreFile.GetData(), ignoreFile.GetElementCount()));
  }

  return EZ_SUCCESS;
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

void ezQtUiServices::ShowAllDocumentsStatusBarMessage(const ezFormatString& msg, ezTime timeOut)
{
  ezStringBuilder tmp;

  Event e;
  e.m_Type = Event::ShowDocumentStatusBarText;
  e.m_sText = msg.GetText(tmp);
  e.m_Time = timeOut;

  s_Events.Broadcast(e);
}

void ezQtUiServices::ShowGlobalStatusBarMessage(const ezFormatString& msg)
{
  ezStringBuilder tmp;

  Event e;
  e.m_Type = Event::ShowGlobalStatusBarText;
  e.m_sText = msg.GetText(tmp);
  e.m_Time = ezTime::Seconds(0);

  s_Events.Broadcast(e);
}


bool ezQtUiServices::OpenFileInDefaultProgram(const char* szPath)
{
  return QDesktopServices::openUrl(QUrl::fromLocalFile(szPath));
}

void ezQtUiServices::OpenInExplorer(const char* szPath, bool bIsFile)
{
  QStringList args;

  if (bIsFile)
    args << "/select,";

  args << QDir::toNativeSeparators(szPath);

  QProcess::startDetached("explorer", args);
}

ezStatus ezQtUiServices::OpenInVsCode(const QStringList& arguments)
{
  QString sVsCodeExe = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "Programs/Microsoft VS Code/Code.exe", QStandardPaths::LocateOption::LocateFile);

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
