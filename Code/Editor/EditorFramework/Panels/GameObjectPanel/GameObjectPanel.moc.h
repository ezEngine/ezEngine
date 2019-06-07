#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>

class ezQtSearchWidget;
class ezGameObjectDocument;
struct ezGameObjectEvent;

class EZ_EDITORFRAMEWORK_DLL ezQtGameObjectPanel : public ezQtDocumentPanel
{
  Q_OBJECT

public:
  ezQtGameObjectPanel(QWidget* pParent, ezGameObjectDocument* pDocument, const char* szContextMenuMapping, std::unique_ptr<ezQtDocumentTreeModel> pCustomModel);
  ~ezQtGameObjectPanel();

private Q_SLOTS:
  void OnItemDoubleClicked(const QModelIndex&);
  void OnRequestContextMenu(QPoint pos);
  void OnFilterTextChanged(const QString& text);

private:
  void DocumentSceneEventHandler(const ezGameObjectEvent& e);

protected:
  QWidget* m_pMainWidget;
  ezGameObjectDocument* m_pDocument;
  ezQtDocumentTreeView* m_pTreeWidget;
  ezQtSearchWidget* m_pFilterWidget;
  ezString m_sContextMenuMapping;
};
