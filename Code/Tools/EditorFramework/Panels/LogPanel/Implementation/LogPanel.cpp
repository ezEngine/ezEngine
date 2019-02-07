#include <EditorFrameworkPCH.h>

#include <EditorFramework/Panels/LogPanel/LogPanel.moc.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Models/LogModel.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

#include <QSettings>

EZ_IMPLEMENT_SINGLETON(ezQtLogPanel);

ezQtLogPanel::ezQtLogPanel()
    : ezQtApplicationPanel("Panel.Log")
    , m_SingletonRegistrar(this)
{
  setupUi(this);

  setWindowIcon(ezQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/Log.png"));
  setWindowTitle(QString::fromUtf8(ezTranslate("Panel.Log")));

  EditorLog->GetSearchWidget()->setPlaceholderText(QStringLiteral("Search Editor Log"));
  EngineLog->GetSearchWidget()->setPlaceholderText(QStringLiteral("Search Engine Log"));

  ezGlobalLog::AddLogWriter(ezMakeDelegate(&ezQtLogPanel::LogWriter, this));
  ezEditorEngineProcessConnection::s_Events.AddEventHandler(ezMakeDelegate(&ezQtLogPanel::EngineProcessMsgHandler, this));

  QSettings Settings;
  Settings.beginGroup(QLatin1String("LogPanel"));
  {
    splitter->restoreState(Settings.value("Splitter", splitter->saveState()).toByteArray());
  }
  Settings.endGroup();
}

ezQtLogPanel::~ezQtLogPanel()
{
  QSettings Settings;
  Settings.beginGroup(QLatin1String("LogPanel"));
  {
    Settings.setValue("Splitter", splitter->saveState());
  }
  Settings.endGroup();

  ezGlobalLog::RemoveLogWriter(ezMakeDelegate(&ezQtLogPanel::LogWriter, this));
  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtLogPanel::EngineProcessMsgHandler, this));
}

void ezQtLogPanel::ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case ezToolsProjectEvent::Type::ProjectClosing:
    {
      EditorLog->GetLog()->Clear();
      EngineLog->GetLog()->Clear();
        // fallthrough
      case ezToolsProjectEvent::Type::ProjectOpened:
        setEnabled(e.m_Type == ezToolsProjectEvent::Type::ProjectOpened);
    }
    break;
  }

  ezQtApplicationPanel::ToolsProjectEventHandler(e);
}

void ezQtLogPanel::LogWriter(const ezLoggingEventData& e)
{
  // Can be called from a different thread, but AddLogMsg is thread safe.
  ezLogEntry msg(e);
  EditorLog->GetLog()->AddLogMsg(msg);

  if (msg.m_sTag == "EditorStatus")
  {
    ezQtUiServices::GetSingleton()->ShowAllDocumentsStatusBarMessage(ezFmt(msg.m_sMsg), ezTime::Seconds(5));
  }
}

void ezQtLogPanel::EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case ezEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<ezLogMsgToEditor>())
      {
        const ezLogMsgToEditor* pMsg = static_cast<const ezLogMsgToEditor*>(e.m_pMsg);
        EngineLog->GetLog()->AddLogMsg(pMsg->m_Entry);
      }
    }
    break;

    default:
      return;
  }
}
