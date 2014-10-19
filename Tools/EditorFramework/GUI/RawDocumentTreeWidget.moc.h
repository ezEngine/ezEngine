#pragma once

#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <ToolsFoundation/Object/DocumentObjectTree.h>
#include <QTreeView>

class EZ_EDITORFRAMEWORK_DLL ezRawDocumentTreeWidget : public QTreeView
{
  Q_OBJECT

public:

  ezRawDocumentTreeWidget(QWidget* pParent, const ezDocumentBase* pDocument);


private slots:
  void on_selectionChanged_triggered(const QItemSelection& selected, const QItemSelection& deselected);

private:
  ezRawDocumentTreeModel m_Model;
  const ezDocumentBase* m_pDocument;
};

