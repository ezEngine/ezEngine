#pragma once

#include <GuiFoundation/Basics.h>
#include <QDialog>
#include <Code/Tools/GuiFoundation/ui_ShortcutEditorDlg.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>

struct ezActionDescriptor;

class EZ_GUIFOUNDATION_DLL ezShortcutEditorDlg : public QDialog, public Ui_ShortcutEditor
{
public:
  Q_OBJECT

public:
  ezShortcutEditorDlg(QWidget* parent);
  ~ezShortcutEditorDlg();

  void UpdateTable();

private slots:
  void SlotSelectionChanged();
  void on_KeyEditor_editingFinished();

  void UpdateKeyEdit();

  void on_KeyEditor_keySequenceChanged(const QKeySequence & keySequence);
  void on_ButtonAssign_clicked();
  void on_ButtonRemove_clicked();
  void on_ButtonReset_clicked();

private:
  ezInt32 m_iSelectedAction;
  ezHybridArray<ezActionDescriptor*, 32> m_ActionDescs;
};


