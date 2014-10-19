#pragma once

#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <ToolsFoundation/Object/DocumentObjectTree.h>
#include <QTreeView>

class EZ_EDITORFRAMEWORK_DLL ezRawDocumentTreeWidget : public QTreeView
{
public:

  ezRawDocumentTreeWidget(QWidget* pParent, ezDocumentObjectTree* pTree);


private:
  ezRawDocumentTreeModel m_Model;
};

