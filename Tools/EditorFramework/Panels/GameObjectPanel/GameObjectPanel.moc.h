#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

class ezQtSearchWidget;

class EZ_EDITORFRAMEWORK_DLL ezQtGameObjectPanel : public ezQtDocumentPanel
{
  Q_OBJECT

public:
  ezQtGameObjectPanel(QWidget* pParent, ezGameObjectDocument* pDocument, const char* szContextMenuMapping, std::unique_ptr<ezQtDocumentTreeModel> pCustomModel);
  ~ezQtGameObjectPanel();

private slots:
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
