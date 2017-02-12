#include <PCH.h>
#include <EditorFramework/Panels/LogPanel/LogPanel.moc.h>
#include <EditorFramework/Panels/LogPanel/LogModel.moc.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

#include <QSettings>

EZ_IMPLEMENT_SINGLETON(ezQtLogPanel);

ezQtLogPanel::ezQtLogPanel()
  : ezQtApplicationPanel("Panel.Log")
  , m_SingletonRegistrar(this)
{
  setupUi(this);

  setWindowIcon(ezQtUiServices::GetCachedIconResource(":/EditorFramework/Icons/Log.png"));
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
  ezQtLogModel::LogMsg msg;
  msg.m_sMsg = e.m_szText;
  msg.m_sTag = e.m_szTag;
  msg.m_Type = e.m_EventType;
  msg.m_uiIndentation = e.m_uiIndentation;

  EditorLog->GetLog()->AddLogMsg(msg);

  if (msg.m_sTag == "EditorStatus")
  {
    ezQtUiServices::GetSingleton()->ShowAllDocumentsStatusBarMessage(msg.m_sMsg, ezTime::Seconds(5));
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

        ezQtLogModel::LogMsg msg;
        msg.m_sMsg = pMsg->m_sText;
        msg.m_sTag = pMsg->m_sTag;
        msg.m_Type = (ezLogMsgType::Enum)pMsg->m_iMsgType;
        msg.m_uiIndentation = pMsg->m_uiIndentation;

        EngineLog->GetLog()->AddLogMsg(msg);
      }
    }
    break;

  default:
    return;
  }
}

