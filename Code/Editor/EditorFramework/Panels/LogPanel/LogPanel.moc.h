#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/ui_LogPanel.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class ezQtLogModel;
struct ezLoggingEventData;
class ezPreferences;

/// \brief The application wide panel that shows the engine log output and the editor log output
class EZ_EDITORFRAMEWORK_DLL ezQtLogPanel : public ezQtApplicationPanel, public Ui_LogPanel
{
  Q_OBJECT

  EZ_DECLARE_SINGLETON(ezQtLogPanel);

public:
  ezQtLogPanel(ads::CDockManager* pDockManager);
  ~ezQtLogPanel();

protected:
  virtual void ToolsProjectEventHandler(const ezToolsProjectEvent& e) override;

private Q_SLOTS:
  void OnNewWarningsOrErrors(const char* szText, bool bError);

private:
  void LogWriter(const ezLoggingEventData& e);
  void EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e);
  void UiServiceEventHandler(const ezQtUiServices::Event& e);
  void OnPreferenceChange(ezPreferences* pref);

  ezUInt32 m_uiIgnoredNumErrors = 0;
  ezUInt32 m_uiIgnoreNumWarnings = 0;
  ezUInt32 m_uiKnownNumErrors = 0;
  ezUInt32 m_uiKnownNumWarnings = 0;

  bool m_bCombineLogs = true;
};
