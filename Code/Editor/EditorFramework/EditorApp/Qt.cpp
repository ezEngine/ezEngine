#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>

void ezQtEditorApp::SetStyleSheet()
{
  ezColorGammaUB highlightColor = ezColorScheme::DarkUI(ezColorScheme::Yellow);
  ezColorGammaUB highlightColorDisabled = ezColorScheme::DarkUI(ezColorScheme::Yellow) * 0.5f;
  ezColorGammaUB linkColor = ezColorScheme::LightUI(ezColorScheme::Orange);
  ezColorGammaUB linkVisitedColor = ezColorScheme::LightUI(ezColorScheme::Yellow);

  QApplication::setStyle(QStyleFactory::create("fusion"));
  QPalette palette;

  palette.setColor(QPalette::WindowText, QColor(221, 221, 221));
  palette.setColor(QPalette::Button, QColor(60, 60, 60));
  palette.setColor(QPalette::Light, QColor(62, 62, 62));
  palette.setColor(QPalette::Midlight, QColor(58, 58, 58));
  palette.setColor(QPalette::Dark, QColor(40, 40, 40));
  palette.setColor(QPalette::Mid, QColor(54, 54, 54));
  palette.setColor(QPalette::Text, QColor(221, 221, 221));
  palette.setColor(QPalette::BrightText, QColor(221, 221, 221));
  palette.setColor(QPalette::ButtonText, QColor(221, 221, 221));
  palette.setColor(QPalette::Base, QColor(25, 25, 25));
  palette.setColor(QPalette::Window, QColor(40, 40, 40));
  palette.setColor(QPalette::Shadow, QColor(0, 0, 0));
  palette.setColor(QPalette::Highlight, ezToQtColor(highlightColor));
  palette.setColor(QPalette::HighlightedText, QColor(52, 52, 52));
  palette.setColor(QPalette::Link, ezToQtColor(linkColor));
  palette.setColor(QPalette::LinkVisited, ezToQtColor(linkVisitedColor));
  palette.setColor(QPalette::AlternateBase, QColor(37, 37, 40));
  QBrush NoRoleBrush(QColor(0, 0, 0), Qt::NoBrush);
  palette.setBrush(QPalette::NoRole, NoRoleBrush);
  palette.setColor(QPalette::ToolTipBase, QColor(52, 52, 52));
  palette.setColor(QPalette::ToolTipText, QColor(221, 221, 221));
  palette.setColor(QPalette::PlaceholderText, QColor(200, 200, 200).darker());

  palette.setColor(QPalette::Disabled, QPalette::Window, QColor(25, 25, 25));
  palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(128, 128, 128));
  palette.setColor(QPalette::Disabled, QPalette::Button, QColor(35, 35, 35));
  palette.setColor(QPalette::Disabled, QPalette::Text, QColor(105, 105, 105));
  palette.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255));
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));
  palette.setColor(QPalette::Disabled, QPalette::Highlight, ezToQtColor(highlightColorDisabled));

  QApplication::setPalette(palette);
}

static void QtDebugMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& sQMsg)
{
  QByteArray localMsg = sQMsg.toLocal8Bit();
  ezStringBuilder sMsg = localMsg.constData();

  switch (type)
  {
    case QtDebugMsg:
      ezLog::Debug("|Qt| {0} ({1}:{2}, {3})", sMsg, context.file, context.line, context.function);
      break;
#if QT_VERSION >= 0x050500
    case QtInfoMsg:
      ezLog::Info("|Qt| {0} ({1}:{2}, {3})", sMsg, context.file, context.line, context.function);
      break;
#endif
    case QtWarningMsg:
    {
      // I just hate this pointless message
      if (sMsg.FindSubString("iCCP") != nullptr)
        return;

      ezLog::Warning("|Qt| {0} ({1}:{2}, {3})", sMsg, context.file, context.line, context.function);
      break;
    }
    case QtCriticalMsg:
      // BUG in Qt 6 on winodows. Window classes are not properly unregistered so they leak into the next session and cause a warning.
      if (!sMsg.StartsWith("QApplication::regClass: Registering window class"))
      {
        ezLog::Error("|Qt| {0} ({1}:{2}, {3})", sMsg, context.file, context.line, context.function);
      }
      break;
    case QtFatalMsg:
      EZ_ASSERT_DEBUG("|Qt| {0} ({1}:{2} {3})", sMsg, context.file, context.line, context.function);
      break;
  }
}

void ezQtEditorApp::InitQt(int iArgc, char** pArgv)
{
  qInstallMessageHandler(QtDebugMessageHandler);

  QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

  if (qApp != nullptr)
  {
    m_pQtApplication = qApp;
    bool ok = false;
    const int iCount = m_pQtApplication->property("Shared").toInt(&ok);
    EZ_ASSERT_DEV(ok, "Existing QApplication was not constructed by EZ!");
    m_pQtApplication->setProperty("Shared", QVariant::fromValue(iCount + 1));
  }
  else
  {
    m_iArgc = iArgc;
    m_pQtApplication = new QApplication(m_iArgc, pArgv);
    m_pQtApplication->setProperty("Shared", QVariant::fromValue((int)1));
    QFont font = m_pQtApplication->font();
    // font.setPixelSize(11);
    m_pQtApplication->setFont(font);
  }
}

void ezQtEditorApp::DeInitQt()
{
  const int iCount = m_pQtApplication->property("Shared").toInt();
  if (iCount == 1)
  {
    delete m_pQtApplication;
  }
  else
  {
    m_pQtApplication->setProperty("Shared", QVariant::fromValue(iCount - 1));
  }
}
