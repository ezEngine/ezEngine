#pragma once

#include <GuiFoundation/Basics.h>
#include <QDialog>
#include <Code/Tools/GuiFoundation/ui_ModifiedDocumentsDlg.h>
#include <ToolsFoundation/Document/Document.h>

class EZ_GUIFOUNDATION_DLL ezModifiedDocumentsDlg : public QDialog, public Ui_DocumentList
{
public:
  Q_OBJECT

public:
  ezModifiedDocumentsDlg(QWidget* parent, const ezHybridArray<ezDocumentBase*, 32>& ModifiedDocs);


private slots:
  void on_ButtonSaveSelected_clicked();
  void on_ButtonDontSave_clicked();
  void on_ButtonCancel_clicked();
  void SlotSaveDocument();
  void SlotSelectionChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

private:
  ezResult SaveDocument(ezDocumentBase* pDoc);

  ezHybridArray<ezDocumentBase*, 32> m_ModifiedDocs;
};


