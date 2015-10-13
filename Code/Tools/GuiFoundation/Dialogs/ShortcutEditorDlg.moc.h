#pragma once

#include <GuiFoundation/Basics.h>
#include <QDialog>
#include <Code/Tools/GuiFoundation/ui_ShortcutEditorDlg.h>
#include <Foundation/Containers/Deque.h>

struct ezActionDescriptor;

class EZ_GUIFOUNDATION_DLL ezShortcutEditorDlg : public QDialog, public Ui_ShortcutEditor
{
public:
  Q_OBJECT

public:
  ezShortcutEditorDlg(QWidget* parent);

  void UpdateTable(bool bOnlyShortcuts);

private slots:
  void SlotSelectionChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
  void on_KeyEditor_editingFinished();

private:
  ezInt32 m_iSelectedAction;
  ezDeque<ezActionDescriptor*> m_ActionDescs;
};


