#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>

class ezQtSearchWidget;
class ezGameObjectDocument;
struct ezGameObjectEvent;
class ezQtGameObjectDelegate;

class EZ_EDITORFRAMEWORK_DLL ezQtGameObjectWidget : public QWidget
{
  Q_OBJECT

public:
  ezQtGameObjectWidget(QWidget* pParent, ezGameObjectDocument* pDocument, const char* szContextMenuMapping, std::unique_ptr<ezQtDocumentTreeModel> pCustomModel, ezSelectionManager* pSelection = nullptr);
  ~ezQtGameObjectWidget();

  ezQtSearchWidget& GetFilterWidget() { return *m_pFilterWidget; }

private Q_SLOTS:
  void OnItemDoubleClicked(const QModelIndex&);
  void OnRequestContextMenu(QPoint pos);
  void OnFilterTextChanged(const QString& text);

private:
  void DocumentSceneEventHandler(const ezGameObjectEvent& e);

protected:
  ezQtGameObjectDelegate* m_pDelegate = nullptr;
  ezGameObjectDocument* m_pDocument;
  ezQtDocumentTreeView* m_pTreeWidget;
  ezQtSearchWidget* m_pFilterWidget;
  ezString m_sContextMenuMapping;
};

class EZ_EDITORFRAMEWORK_DLL ezQtGameObjectPanel : public ezQtDocumentPanel
{
  Q_OBJECT

public:
  ezQtGameObjectPanel(ads::CDockManager* pDockManager, QWidget* pParent, ezGameObjectDocument* pDocument, const char* szContextMenuMapping, std::unique_ptr<ezQtDocumentTreeModel> pCustomModel);
  ~ezQtGameObjectPanel();


protected:
  ezQtGameObjectWidget* m_pMainWidget = nullptr;
};
