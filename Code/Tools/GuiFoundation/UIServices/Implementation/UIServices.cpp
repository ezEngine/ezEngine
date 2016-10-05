#include <GuiFoundation/PCH.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QSettings>
#include <QProcess>
#include <QDir>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QUrl>
#include <QDesktopServices>

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
    Settings.setValue("ColorDlgPos", m_ColorDlgPos);
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

  map[sIdentifier] = QIcon(QString::fromUtf8(szIdentifier));

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

void ezQtUiServices::LoadState()
{
  QSettings Settings;
  Settings.beginGroup("EditorGUI");
  {
    m_ColorDlgPos = Settings.value("ColorDlgPos", QPoint(100, 100)).toPoint();
  }
  Settings.endGroup();
}

void ezQtUiServices::ShowAllDocumentsStatusBarMessage(const char* szMsg, ezTime timeOut)
{
  Event e;
  e.m_Type = Event::ShowDocumentStatusBarText;
  e.m_sText = szMsg;
  e.m_Time = timeOut;

  s_Events.Broadcast(e);
}

void ezQtUiServices::ShowGlobalStatusBarMessage(const char* szMsg)
{
  Event e;
  e.m_Type = Event::ShowGlobalStatusBarText;
  e.m_sText = szMsg;
  e.m_Time = ezTime::Seconds(0);

  s_Events.Broadcast(e);
}


bool ezQtUiServices::OpenFileInDefaultProgram(const char* szPath)
{
  return QDesktopServices::openUrl(QUrl(szPath));
}

void ezQtUiServices::OpenInExplorer(const char* szPath)
{
  QStringList args;
  args << "/select," << QDir::toNativeSeparators(szPath);
  QProcess::startDetached("explorer", args);
}

