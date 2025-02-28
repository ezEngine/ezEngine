#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Panels/LogPanel/LogPanel.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <GuiFoundation/Models/LogModel.moc.h>


EZ_IMPLEMENT_SINGLETON(ezQtLogPanel);

ezQtLogPanel::ezQtLogPanel(ads::CDockManager* pDockManager)
  : ezQtApplicationPanel(pDockManager, "Panel.Log")
  , m_SingletonRegistrar(this)
{
  QWidget* pDummy = new QWidget();
  setupUi(pDummy);
  pDummy->setContentsMargins(0, 0, 0, 0);
  pDummy->layout()->setContentsMargins(0, 0, 0, 0);

  setIcon(ezQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/Log.svg"));
  setWindowTitle(ezMakeQString(ezTranslate("Panel.Log")));
  setWidget(pDummy);

  EditorLog->GetSearchWidget()->setPlaceholderText(QStringLiteral("Search Editor Log"));
  EngineLog->GetSearchWidget()->setPlaceholderText(QStringLiteral("Search Engine Log"));
  CombinedLog->GetSearchWidget()->setPlaceholderText(QStringLiteral("Search Log"));

  ezGlobalLog::AddLogWriter(ezMakeDelegate(&ezQtLogPanel::LogWriter, this));
  ezEditorEngineProcessConnection::s_Events.AddEventHandler(ezMakeDelegate(&ezQtLogPanel::EngineProcessMsgHandler, this));

  QSettings Settings;
  Settings.beginGroup(QLatin1String("LogPanel"));
  {
    splitter->restoreState(Settings.value("Splitter", splitter->saveState()).toByteArray());
  }
  Settings.endGroup();

  connect(EditorLog->GetLog(), &ezQtLogModel::NewErrorsOrWarnings, this, &ezQtLogPanel::OnNewWarningsOrErrors);
  connect(EngineLog->GetLog(), &ezQtLogModel::NewErrorsOrWarnings, this, &ezQtLogPanel::OnNewWarningsOrErrors);
  connect(CombinedLog->GetLog(), &ezQtLogModel::NewErrorsOrWarnings, this, &ezQtLogPanel::OnNewWarningsOrErrors);

  ezQtUiServices::GetSingleton()->s_Events.AddEventHandler(ezMakeDelegate(&ezQtLogPanel::UiServiceEventHandler, this));

  ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
  pPreferences->m_ChangedEvent.AddEventHandler(ezMakeDelegate(&ezQtLogPanel::OnPreferenceChange, this));

  m_bCombineLogs = pPreferences->m_bCombinedEditorAndEngineLogs;

  LogWidgets->setCurrentIndex(m_bCombineLogs ? 0 : 1);
}

ezQtLogPanel::~ezQtLogPanel()
{
  ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
  pPreferences->m_ChangedEvent.RemoveEventHandler(ezMakeDelegate(&ezQtLogPanel::OnPreferenceChange, this));

  QSettings Settings;
  Settings.beginGroup(QLatin1String("LogPanel"));
  {
    Settings.setValue("Splitter", splitter->saveState());
  }
  Settings.endGroup();

  ezGlobalLog::RemoveLogWriter(ezMakeDelegate(&ezQtLogPanel::LogWriter, this));
  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtLogPanel::EngineProcessMsgHandler, this));
  ezQtUiServices::GetSingleton()->s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtLogPanel::UiServiceEventHandler, this));
}

void ezQtLogPanel::OnNewWarningsOrErrors(const char* szText, bool bError)
{
  m_uiKnownNumWarnings = EditorLog->GetLog()->GetNumSeriousWarnings() + EditorLog->GetLog()->GetNumWarnings() +
                         EngineLog->GetLog()->GetNumSeriousWarnings() + EngineLog->GetLog()->GetNumWarnings() +
                         CombinedLog->GetLog()->GetNumSeriousWarnings() + CombinedLog->GetLog()->GetNumWarnings();
  m_uiKnownNumErrors = EditorLog->GetLog()->GetNumErrors() + EngineLog->GetLog()->GetNumErrors() + CombinedLog->GetLog()->GetNumErrors();

  ezQtUiServices::Event::TextType type = ezQtUiServices::Event::Info;

  ezUInt32 uiShowNumWarnings = 0;
  ezUInt32 uiShowNumErrors = 0;

  if (m_uiKnownNumWarnings > m_uiIgnoreNumWarnings)
  {
    uiShowNumWarnings = m_uiKnownNumWarnings - m_uiIgnoreNumWarnings;
    type = ezQtUiServices::Event::Warning;
  }
  else
  {
    m_uiIgnoreNumWarnings = m_uiKnownNumWarnings;
  }

  if (m_uiKnownNumErrors > m_uiIgnoredNumErrors)
  {
    uiShowNumErrors = m_uiKnownNumErrors - m_uiIgnoredNumErrors;
    type = ezQtUiServices::Event::Error;
  }
  else
  {
    m_uiIgnoredNumErrors = m_uiKnownNumErrors;
  }

  ezStringBuilder tmp;
  if (uiShowNumErrors > 0)
  {
    tmp.AppendFormat("{} Errors", uiShowNumErrors);
  }
  if (uiShowNumWarnings > 0)
  {
    tmp.AppendWithSeparator(",", " ");
    tmp.AppendFormat("{} Warnings", uiShowNumWarnings);
  }

  ezQtUiServices::GetSingleton()->ShowAllDocumentsPermanentStatusBarMessage(tmp, type);

  if (!ezStringUtils::IsNullOrEmpty(szText))
  {
    ezQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(
      ezFmt("{}: {}", bError ? "Error" : "Warning", szText), ezTime::MakeFromSeconds(10));
  }
}

void ezQtLogPanel::ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case ezToolsProjectEvent::Type::ProjectClosing:
      CombinedLog->GetLog()->Clear();
      EditorLog->GetLog()->Clear();
      EngineLog->GetLog()->Clear();
      [[fallthrough]];

    case ezToolsProjectEvent::Type::ProjectOpened:
      setEnabled(e.m_Type == ezToolsProjectEvent::Type::ProjectOpened);
      break;

    default:
      break;
  }

  ezQtApplicationPanel::ToolsProjectEventHandler(e);
}

void ezQtLogPanel::LogWriter(const ezLoggingEventData& e)
{
  // Can be called from a different thread, but AddLogMsg is thread safe.
  ezLogEntry msg(e);

  if (m_bCombineLogs)
    CombinedLog->GetLog()->AddLogMsg(msg);
  else
    EditorLog->GetLog()->AddLogMsg(msg);

  if (msg.m_sTag == "EditorStatus")
  {
    ezQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(ezFmt(msg.m_sMsg), ezTime::MakeFromSeconds(5));
  }
}

void ezQtLogPanel::EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case ezEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      if (const ezLogMsgToEditor* pMsg = ezDynamicCast<const ezLogMsgToEditor*>(e.m_pMsg))
      {
        if (m_bCombineLogs)
          CombinedLog->GetLog()->AddLogMsg(pMsg->m_Entry);
        else
          EngineLog->GetLog()->AddLogMsg(pMsg->m_Entry);
      }
    }
    break;

    default:
      return;
  }
}

void ezQtLogPanel::UiServiceEventHandler(const ezQtUiServices::Event& e)
{
  if (e.m_Type == ezQtUiServices::Event::ClickedDocumentPermanentStatusBarText)
  {
    EnsureVisible();

    m_uiIgnoredNumErrors = m_uiKnownNumErrors;
    m_uiIgnoreNumWarnings = m_uiKnownNumWarnings;

    ezQtUiServices::GetSingleton()->ShowAllDocumentsPermanentStatusBarMessage(nullptr, ezQtUiServices::Event::Info);
  }
}

void ezQtLogPanel::OnPreferenceChange(ezPreferences* pref)
{
  if (ezEditorPreferencesUser* pPref = ezDynamicCast<ezEditorPreferencesUser*>(pref))
  {
    if (m_bCombineLogs != pPref->m_bCombinedEditorAndEngineLogs)
    {
      m_bCombineLogs = pPref->m_bCombinedEditorAndEngineLogs;

      LogWidgets->setCurrentIndex(m_bCombineLogs ? 0 : 1);
    }
  }
}
