#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <QDialog>
#include <Code/Tools/GuiFoundation/ui_ModifiedDocumentsDlg.h>
#include <ToolsFoundation/Document/Document.h>

class EZ_GUIFOUNDATION_DLL ezQtModifiedDocumentsDlg : public QDialog, public Ui_DocumentList
{
public:
  Q_OBJECT

public:
  ezQtModifiedDocumentsDlg(QWidget* parent, const ezHybridArray<ezDocument*, 32>& ModifiedDocs);


private Q_SLOTS:
  void on_ButtonSaveSelected_clicked();
  void on_ButtonDontSave_clicked();
  void SlotSaveDocument();
  void SlotSelectionChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

private:
  ezResult SaveDocument(ezDocument* pDoc);

  ezHybridArray<ezDocument*, 32> m_ModifiedDocs;
};


