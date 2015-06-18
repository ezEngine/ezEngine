#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <GuiFoundation/DockWindow/DockWindow.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Tools/EditorPluginScene/ui_LogPanelWidget.h>
#include <Foundation/Logging/Log.h>

class ezLogPanel : public ezApplicationPanel, public Ui_LogPanel
{
  Q_OBJECT

public:
  ezLogPanel();
  ~ezLogPanel();

private:
  void LogWriter(const ezLoggingEventData& e);
  void EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e);
};