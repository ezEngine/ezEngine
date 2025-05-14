#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Scripting/LuaWrapper.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Logging/Log.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageUtils.h>

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

int lua_SetColor(lua_State* s)
{
  ezLuaWrapper lua(s);

  QPalette* palette = (QPalette*)lua.GetFunctionLightUserData();

  const int iColor = lua.GetIntParameter(0);
  const int r = lua.GetIntParameter(1);
  const int g = lua.GetIntParameter(2);
  const int b = lua.GetIntParameter(3);

  const QPalette::ColorRole role = (QPalette::ColorRole)iColor;

  palette->setColor(role, QColor(r, g, b));

  return lua.ReturnToScript();
}

int lua_SetDisabledColor(lua_State* s)
{
  ezLuaWrapper lua(s);

  QPalette* palette = (QPalette*)lua.GetFunctionLightUserData();

  const int iColor = lua.GetIntParameter(0);
  const int r = lua.GetIntParameter(1);
  const int g = lua.GetIntParameter(2);
  const int b = lua.GetIntParameter(3);

  const QPalette::ColorRole role = (QPalette::ColorRole)iColor;

  palette->setColor(QPalette::Disabled, role, QColor(r, g, b));

  return lua.ReturnToScript();
}

#endif

void ezQtEditorApp::SetStyleSheet()
{
  QPalette palette;

  ezColorGammaUB highlightColor = ezColorScheme::DarkUI(ezColorScheme::Yellow);
  ezColorGammaUB highlightColorDisabled = ezColorScheme::DarkUI(ezColorScheme::Yellow) * 0.5f;
  ezColorGammaUB linkVisitedColor = ezColorScheme::LightUI(ezColorScheme::Yellow);

  QApplication::setStyle(QStyleFactory::create("fusion"));

  QBrush NoRoleBrush(QColor(0, 0, 0), Qt::NoBrush);
  palette.setBrush(QPalette::NoRole, NoRoleBrush);

  palette.setColor(QPalette::WindowText, QColor(255, 255, 255));          // labels, tabs, property grid
  palette.setColor(QPalette::Button, QColor(51, 51, 51));                 // buttons, toolbuttons, dashboard background
  palette.setColor(QPalette::Light, QColor(81, 58, 47));                  // lines between tabs, inactive tab gradient
  palette.setColor(QPalette::Dark, QColor(0, 0, 0));                      // line below active window highlight
  palette.setColor(QPalette::Mid, QColor(90, 90, 90));                    // color of the box around component properties (collapsible group box)
  palette.setColor(QPalette::Text, QColor(204, 204, 204));                // scene graph, values in spin boxes, checkmarks
  palette.setColor(QPalette::ButtonText, QColor(204, 204, 204));          // menus, comboboxes, headers
  palette.setColor(QPalette::Base, QColor(24, 24, 24));                   // background inside complex windows (scenegraph)
  palette.setColor(QPalette::Window, QColor(42, 42, 42));                 // window borders, toolbars
  palette.setColor(QPalette::Shadow, QColor(70, 70, 70));                 // background color for arrays in property grids
  palette.setColor(QPalette::Highlight, ezToQtColor(highlightColor));     // selected items
  palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));     // text of selected items
  palette.setColor(QPalette::Link, QColor(104, 205, 254));                // manipulator links in property grid
  palette.setColor(QPalette::LinkVisited, ezToQtColor(linkVisitedColor)); // manipulator links in property grid when active
  palette.setColor(QPalette::AlternateBase, QColor(49, 49, 49));          // second base color, mainly used for alternate row colors
  palette.setColor(QPalette::PlaceholderText, QColor(142, 142, 142));     // text in search fields

  palette.setColor(QPalette::Midlight, QColor(58, 58, 58));               // unused ?
  palette.setColor(QPalette::BrightText, QColor(221, 221, 221));          // unused ?
  palette.setColor(QPalette::ToolTipBase, QColor(52, 52, 52));            // unused / not working ?
  palette.setColor(QPalette::ToolTipText, QColor(221, 221, 221));         // unused / not working ?

  palette.setColor(QPalette::Disabled, QPalette::Window, QColor(25, 25, 25));
  palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(128, 128, 128));
  palette.setColor(QPalette::Disabled, QPalette::Button, QColor(35, 35, 35));
  palette.setColor(QPalette::Disabled, QPalette::Text, QColor(105, 105, 105));
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));
  palette.setColor(QPalette::Disabled, QPalette::Highlight, ezToQtColor(highlightColorDisabled));

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT
  if (false)
  {
    // when enabled, you can edit the palette with a Lua file
    // see ezProjectAction::Execute(), case ezProjectAction::ButtonType::ReloadResources:
    // to enable reloading on "reload resources"
    // Example Lua file:
    // SetColor(Base, 24, 24, 24)
    // SetDisabledColor(Base, 5, 5, 5)

    ezOSFile file;
    if (file.Open("D:\\Style.lua", ezFileOpenMode::Read).Succeeded())
    {
      ezDataBuffer content;
      file.ReadAll(content);
      content.PushBack('\0');

      ezLuaWrapper lua;
      lua.SetVariable("WindowText", QPalette::WindowText);
      lua.SetVariable("Button", QPalette::Button);
      lua.SetVariable("Light", QPalette::Light);
      lua.SetVariable("Midlight", QPalette::Midlight);
      lua.SetVariable("Dark", QPalette::Dark);
      lua.SetVariable("Mid", QPalette::Mid);
      lua.SetVariable("Text", QPalette::Text);
      lua.SetVariable("BrightText", QPalette::BrightText);
      lua.SetVariable("ButtonText", QPalette::ButtonText);
      lua.SetVariable("Base", QPalette::Base);
      lua.SetVariable("Window", QPalette::Window);
      lua.SetVariable("Shadow", QPalette::Shadow);
      lua.SetVariable("Highlight", QPalette::Highlight);
      lua.SetVariable("HighlightedText", QPalette::HighlightedText);
      lua.SetVariable("Link", QPalette::Link);
      lua.SetVariable("LinkVisited", QPalette::LinkVisited);
      lua.SetVariable("AlternateBase", QPalette::AlternateBase);
      lua.SetVariable("ToolTipBase", QPalette::ToolTipBase);
      lua.SetVariable("ToolTipText", QPalette::ToolTipText);
      lua.SetVariable("PlaceholderText", QPalette::PlaceholderText);
      lua.RegisterCFunction("SetColor", lua_SetColor, &palette);
      lua.RegisterCFunction("SetDisabledColor", lua_SetDisabledColor, &palette);

      lua.ExecuteString((const char*)content.GetData(), "", ezLog::GetThreadLocalLogSystem()).IgnoreResult();
    }
  }
#endif

  QApplication::setPalette(palette);
}

static void QtDebugMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& sQMsg)
{
  QByteArray localMsg = sQMsg.toUtf8();
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
      // BUG in Qt 6 on Windows. Window classes are not properly unregistered so they leak into the next session and cause a warning.
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
    // Locale fixes required by various third party libraries like RmlGui.
    QLocale::setDefault(QLocale::C);
    const char* locales[] = {"C.UTF-8", "C.utf8", "UTF-8"};
    for (const char* szLocale : locales)
    {
      if (setlocale(LC_ALL, szLocale) != nullptr)
        break;
    }
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
