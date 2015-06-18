#include <PCH.h>
#include <EditorPluginScene/Panels/LogPanel/LogPanel.moc.h>

ezLogPanel::ezLogPanel()
{
  setObjectName("LogPanel");
  setWindowTitle("Log");

  setupUi(this);

  ezGlobalLog::AddLogWriter(ezMakeDelegate(&ezLogPanel::LogWriter, this));
  ezEditorEngineProcessConnection::s_Events.AddEventHandler(ezMakeDelegate(&ezLogPanel::EngineProcessMsgHandler, this));
}

ezLogPanel::~ezLogPanel()
{
  ezGlobalLog::RemoveLogWriter(ezMakeDelegate(&ezLogPanel::LogWriter, this));
  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(ezMakeDelegate(&ezLogPanel::EngineProcessMsgHandler, this));
}

void ezLogPanel::LogWriter(const ezLoggingEventData& e)
{
  switch (e.m_EventType)
  {
  case ezLogMsgType::DebugMsg:
  case ezLogMsgType::DevMsg:
  case ezLogMsgType::ErrorMsg:
  case ezLogMsgType::InfoMsg:
  case ezLogMsgType::SeriousWarningMsg:
  case ezLogMsgType::SuccessMsg:
  case ezLogMsgType::WarningMsg:
    MsgList->addItem(QString::fromUtf8(e.m_szText));
    break;
  }
}

void ezLogPanel::EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
  case ezEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<ezLogMsgToEditor>())
      {
        const ezLogMsgToEditor* pMsg = static_cast<const ezLogMsgToEditor*>(e.m_pMsg);

        MsgList->addItem(QString::fromUtf8(pMsg->m_sText));
      }
    }
    break;

  default:
    return;
  }
}
