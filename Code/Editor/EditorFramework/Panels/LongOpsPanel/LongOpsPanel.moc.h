#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <Editor/EditorFramework/ui_LongOpsPanel.h>

#include <QTimer>

struct ezToolsProjectEvent;
struct ezLongOperationManagerEvent;

/// \brief The application wide panel that shows the engine log output and the editor log output
class EZ_EDITORFRAMEWORK_DLL ezQtLongOpsPanel : public ezQtApplicationPanel, public Ui_LongOpsPanel
{
  Q_OBJECT

  EZ_DECLARE_SINGLETON(ezQtLongOpsPanel);

public:
  ezQtLongOpsPanel();
  ~ezQtLongOpsPanel();

private:
  void LongOpsEventHandler(const ezLongOperationManagerEvent& e);
  void RebuildTable();

  QTimer m_HandleQueueTimer;
  ezDynamicArray<ezLongOperationManagerEvent> m_EventQueue;
  ezHashTable<ezUInt32, ezUInt32> m_LongOpIdxToRow;

private Q_SLOTS:
  void HandleEventQueue();
};
