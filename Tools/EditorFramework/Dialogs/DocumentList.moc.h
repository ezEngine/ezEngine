#pragma once

#include <Foundation/Basics.h>
#include <QDialog>
#include <Tools/EditorFramework/ui_DocumentList.h>
#include <ToolsFoundation/Document/Document.h>

class DocumentList : public QDialog, public Ui_DocumentList
{
public:
  Q_OBJECT

public:
  DocumentList(QWidget* parent, const ezHybridArray<ezDocumentBase*, 32>& ModifiedDocs);


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


