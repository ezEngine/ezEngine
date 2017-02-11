#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

class ezQtSearchWidget;

class ezQtScenegraphPanel : public ezQtDocumentPanel
{
  Q_OBJECT

public:
  ezQtScenegraphPanel(QWidget* pParent, ezSceneDocument* pDocument);
  ~ezQtScenegraphPanel();

  static void RegisterActions();

private slots:
  void OnItemDoubleClicked(const QModelIndex&);
  void OnRequestContextMenu(QPoint pos);
  void OnFilterTextChanged(const QString& text);

private:
  void DocumentSceneEventHandler(const ezSceneDocumentEvent& e);

  QWidget* m_pMainWidget;
  ezSceneDocument* m_pDocument;
  ezQtDocumentTreeView* m_pTreeWidget;
  ezQtSearchWidget* m_pFilterWidget;
};