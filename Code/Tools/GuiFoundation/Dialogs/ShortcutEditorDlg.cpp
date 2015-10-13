#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Dialogs/ShortcutEditorDlg.moc.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <CoreUtils/Localization/TranslationLookup.h>
#include <QTableWidget>
#include <QKeySequenceEdit>

ezShortcutEditorDlg::ezShortcutEditorDlg(QWidget* parent) : QDialog(parent)
{
  setupUi(this);

  EZ_VERIFY(connect(Shortcuts, SIGNAL(currentCellChanged(int, int, int, int)), this, SLOT(SlotSelectionChanged(int, int, int, int))) != nullptr, "signal/slot connection failed");

  m_iSelectedAction = -1;
  KeyEditor->setEnabled(false);

  {
    auto itActions = ezActionManager::GetActionIterator();

    while (itActions.IsValid())
    {
      if (itActions.Value()->m_Type == ezActionType::Action)
        m_ActionDescs.PushBack(itActions.Value());

      itActions.Next();
    }
  }

  {
    QtScopedBlockSignals bs(Shortcuts);
    QtScopedUpdatesDisabled ud(Shortcuts);

    Shortcuts->setRowCount(m_ActionDescs.GetCount());
    Shortcuts->setColumnCount(3);

    UpdateTable(false);

    Shortcuts->resizeColumnsToContents();
  }
}

void ezShortcutEditorDlg::UpdateTable(bool bOnlyShortcuts)
{
  ezInt32 iRow = 0;
  for (auto pDesc : m_ActionDescs)
  {
    if (!bOnlyShortcuts)
    {
      QTableWidgetItem* pItem0 = new QTableWidgetItem();
      pItem0->setData(Qt::DisplayRole, QVariant(pDesc->m_sActionName.GetData()));
      Shortcuts->setItem(iRow, 0, pItem0);

      QTableWidgetItem* pItem1 = new QTableWidgetItem();
      pItem1->setData(Qt::DisplayRole, QVariant(ezTranslate(pDesc->m_sActionName)));
      Shortcuts->setItem(iRow, 1, pItem1);
    }

    QTableWidgetItem* pItem2 = new QTableWidgetItem();
    pItem2->setData(Qt::DisplayRole, QVariant(pDesc->m_sShortcut.GetData()));
    Shortcuts->setItem(iRow, 2, pItem2);

    ++iRow;
  }
}

void ezShortcutEditorDlg::SlotSelectionChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
  if (currentRow >= 0 && currentRow < (int) m_ActionDescs.GetCount())
  {
    m_iSelectedAction = currentRow;
    KeyEditor->setKeySequence(QKeySequence(m_ActionDescs[currentRow]->m_sShortcut.GetData()));
    KeyEditor->setEnabled(true);
  }
  else
  {
    m_iSelectedAction = -1;
    KeyEditor->clear();
    KeyEditor->setEnabled(false);
  }
}

void ezShortcutEditorDlg::on_KeyEditor_editingFinished()
{
  if (m_iSelectedAction < 0)
    return;
 
  QString sText = KeyEditor->keySequence().toString(QKeySequence::SequenceFormat::NativeText);

  m_ActionDescs[m_iSelectedAction]->m_sShortcut = sText.toUtf8().data();
  m_ActionDescs[m_iSelectedAction]->UpdateExistingActions();

  UpdateTable(true);
}



