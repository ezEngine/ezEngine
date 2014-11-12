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
  virtual void on_ButtonSaveAll_clicked();
  virtual void on_ButtonDiscardAll_clicked();
  virtual void on_ButtonCancel_clicked();

private:

};


