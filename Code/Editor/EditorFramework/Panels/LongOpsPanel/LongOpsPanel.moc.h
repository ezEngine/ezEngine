#pragma once

#include <Editor/EditorFramework/ui_LongOpsPanel.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>

#include <QTimer>

struct ezLongOpControllerEvent;

/// \brief This panel listens to events from ezLongOpControllerManager and displays all currently known long operations
class EZ_EDITORFRAMEWORK_DLL ezQtLongOpsPanel : public ezQtApplicationPanel, public Ui_LongOpsPanel
{
  Q_OBJECT

  EZ_DECLARE_SINGLETON(ezQtLongOpsPanel);

public:
  ezQtLongOpsPanel();
  ~ezQtLongOpsPanel();

private:
  void LongOpsEventHandler(const ezLongOpControllerEvent& e);
  void RebuildTable();
  void UpdateTable();

  bool m_bUpdateTimerRunning = false;
  bool m_bRebuildTable = true;
  bool m_bUpdateTable = false;
  ezHashTable<ezUuid, ezUInt32> m_LongOpGuidToRow;


private Q_SLOTS:
  void StartUpdateTimer();
  void UpdateUI();
  void OnClickButton(bool);
  void OnCellDoubleClicked(int row, int column);
};
