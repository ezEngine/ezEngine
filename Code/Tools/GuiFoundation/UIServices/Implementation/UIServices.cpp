#include <GuiFoundation/PCH.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QSettings>
#include <QProcess>
#include <QDir>
#include <QIcon>
#include <QImage>
#include <QPixmap>

ezEvent<const ezUIServices::Event&> ezUIServices::s_Events;
ezMap<ezString, QIcon> ezUIServices::s_IconsCache;
ezMap<ezString, QImage> ezUIServices::s_ImagesCache;
ezMap<ezString, QPixmap> ezUIServices::s_PixmapsCache;


ezUIServices* ezUIServices::GetInstance()
{
  static ezUIServices instance;
  return &instance;
}

ezUIServices::ezUIServices()
{
  m_pColorDlg = nullptr;
}

void ezUIServices::SaveState()
{
  QSettings Settings;
  Settings.beginGroup("EditorGUI");
  {
    Settings.setValue("ColorDlgPos", m_ColorDlgPos);
  }
  Settings.endGroup();
}


const QIcon& ezUIServices::GetCachedIconResource(const char* szIdentifier)
{
  const ezString sIdentifier = szIdentifier;
  auto& map = s_IconsCache;

  auto it = map.Find(sIdentifier);

  if (it.IsValid())
    return it.Value();

  map[sIdentifier] = QIcon(QString::fromUtf8(szIdentifier));

  return map[sIdentifier];
}


const QImage& ezUIServices::GetCachedImageResource(const char* szIdentifier)
{
  const ezString sIdentifier = szIdentifier;
  auto& map = s_ImagesCache;

  auto it = map.Find(sIdentifier);

  if (it.IsValid())
    return it.Value();

  map[sIdentifier] = QImage(QString::fromUtf8(szIdentifier));

  return map[sIdentifier];
}


const QPixmap& ezUIServices::GetCachedPixmapResource(const char* szIdentifier)
{
  const ezString sIdentifier = szIdentifier;
  auto& map = s_PixmapsCache;

  auto it = map.Find(sIdentifier);

  if (it.IsValid())
    return it.Value();

  map[sIdentifier] = QPixmap(QString::fromUtf8(szIdentifier));

  return map[sIdentifier];
}

void ezUIServices::LoadState()
{
  QSettings Settings;
  Settings.beginGroup("EditorGUI");
  {
    m_ColorDlgPos = Settings.value("ColorDlgPos", QPoint(100, 100)).toPoint();
  }
  Settings.endGroup();
}

void ezUIServices::ShowAllDocumentsStatusBarMessage(const char* szMsg, ezTime timeOut)
{
  Event e;
  e.m_Type = Event::ShowDocumentStatusBarText;
  e.m_sText = szMsg;
  e.m_Time = timeOut;

  s_Events.Broadcast(e);
}

void ezUIServices::ShowGlobalStatusBarMessage(const char* szMsg)
{
  Event e;
  e.m_Type = Event::ShowGlobalStatusBarText;
  e.m_sText = szMsg;
  e.m_Time = ezTime::Seconds(0);

  s_Events.Broadcast(e);
}

void ezUIServices::OpenInExplorer(const char* szPath)
{
  QStringList args;
  args << "/select," << QDir::toNativeSeparators(szPath);
  QProcess::startDetached("explorer", args);
}

