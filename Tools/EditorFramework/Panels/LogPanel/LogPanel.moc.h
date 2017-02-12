#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/Plugin.h>
#include <Tools/EditorFramework/ui_LogPanel.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class ezQtLogModel;
struct ezLoggingEventData;

/// \brief The application wide panel that shows the engine log output and the editor log output
class EZ_EDITORFRAMEWORK_DLL ezQtLogPanel : public ezQtApplicationPanel, public Ui_LogPanel
{
  Q_OBJECT

  EZ_DECLARE_SINGLETON(ezQtLogPanel);

public:
  ezQtLogPanel();
  ~ezQtLogPanel();

protected:
  virtual void ToolsProjectEventHandler(const ezToolsProjectEvent& e) override;

private:
  void LogWriter(const ezLoggingEventData& e);
  void EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e);
};
